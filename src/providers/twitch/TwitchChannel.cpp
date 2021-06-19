#include "providers/twitch/TwitchChannel.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/Message.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/api/Kraken.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
#include "util/PostToThread.hpp"
#include "widgets/Window.hpp"

#include <rapidjson/document.h>
#include <IrcConnection>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QTimer>
#include "common/QLogging.hpp"

namespace chatterino {
namespace {
    constexpr char MAGIC_MESSAGE_SUFFIX[] = u8" \U000E0000";
    constexpr int TITLE_REFRESH_PERIOD = 10000;
    constexpr int CLIP_CREATION_COOLDOWN = 5000;
    const QString CLIPS_LINK("https://clips.twitch.tv/%1");
    const QString CLIPS_FAILURE_CLIPS_DISABLED_TEXT(
        "Failed to create a clip - the streamer has clips disabled entirely or "
        "requires a certain subscriber or follower status to create clips.");
    const QString CLIPS_FAILURE_NOT_AUTHENTICATED_TEXT(
        "Failed to create a clip - you need to re-authenticate.");
    const QString CLIPS_FAILURE_UNKNOWN_ERROR_TEXT(
        "Failed to create a clip - an unknown error occurred.");
    const QString LOGIN_PROMPT_TEXT("Click here to add your account again.");
    const Link ACCOUNTS_LINK(Link::OpenAccountsPage, QString());

    // convertClearchatToNotice takes a Communi::IrcMessage that is a CLEARCHAT command and converts it to a readable NOTICE message
    // This has historically been done in the Recent Messages API, but this functionality is being moved to Chatterino instead
    auto convertClearchatToNotice(Communi::IrcMessage *message)
    {
        auto channelName = message->parameter(0);
        QString noticeMessage{};
        if (message->tags().contains("target-user-id"))
        {
            auto target = message->parameter(1);

            if (message->tags().contains("ban-duration"))
            {
                // User was timed out
                noticeMessage =
                    QString("%1 has been timed out for %2.")
                        .arg(target)
                        .arg(formatTime(
                            message->tag("ban-duration").toString()));
            }
            else
            {
                // User was permanently banned
                noticeMessage =
                    QString("%1 has been permanently banned.").arg(target);
            }
        }
        else
        {
            // Chat was cleared
            noticeMessage = "Chat has been cleared by a moderator.";
        }

        // rebuild the raw irc message so we can convert it back to an ircmessage again!
        // this could probably be done in a smarter way

        auto s = QString(":tmi.twitch.tv NOTICE %1 :%2")
                     .arg(channelName)
                     .arg(noticeMessage);

        auto newMessage = Communi::IrcMessage::fromData(s.toUtf8(), nullptr);
        newMessage->setTags(message->tags());

        return newMessage;
    }

    // parseRecentMessages takes a json object and returns a vector of
    // Communi IrcMessages
    auto parseRecentMessages(const QJsonObject &jsonRoot, ChannelPtr channel)
    {
        QJsonArray jsonMessages = jsonRoot.value("messages").toArray();
        std::vector<Communi::IrcMessage *> messages;

        if (jsonMessages.empty())
            return messages;

        for (const auto jsonMessage : jsonMessages)
        {
            auto content = jsonMessage.toString().toUtf8();

            auto message = Communi::IrcMessage::fromData(content, nullptr);

            if (message->command() == "CLEARCHAT")
            {
                message = convertClearchatToNotice(message);
            }

            messages.emplace_back(std::move(message));
        }

        return messages;
    }
    std::pair<Outcome, std::unordered_set<QString>> parseChatters(
        const QJsonObject &jsonRoot)
    {
        static QStringList categories = {"broadcaster", "vips",   "moderators",
                                         "staff",       "admins", "global_mods",
                                         "viewers"};

        auto usernames = std::unordered_set<QString>();

        // parse json
        QJsonObject jsonCategories = jsonRoot.value("chatters").toObject();

        for (const auto &category : categories)
        {
            for (auto jsonCategory : jsonCategories.value(category).toArray())
            {
                usernames.insert(jsonCategory.toString());
            }
        }

        return {Success, std::move(usernames)};
    }
}  // namespace

TwitchChannel::TwitchChannel(const QString &name, SeventvEmotes &seventv,
                             BttvEmotes &bttv, FfzEmotes &ffz)
    : Channel(name, Channel::Type::Twitch)
    , ChannelChatters(*static_cast<Channel *>(this))
    , nameOptions{name, name}
    , subscriptionUrl_("https://www.twitch.tv/subs/" + name)
    , channelUrl_("https://twitch.tv/" + name)
    , popoutPlayerUrl_("https://player.twitch.tv/?parent=twitch.tv&channel=" +
                       name)
    , globalSeventv_(seventv)
    , globalBttv_(bttv)
    , globalFfz_(ffz)
    , seventvEmotes_(std::make_shared<EmoteMap>())
    , bttvEmotes_(std::make_shared<EmoteMap>())
    , ffzEmotes_(std::make_shared<EmoteMap>())
    , mod_(false)
{
    qCDebug(chatterinoTwitch) << "[TwitchChannel" << name << "] Opened";

    this->managedConnect(getApp()->accounts->twitch.currentUserChanged, [=] {
        this->setMod(false);
    });

    // pubsub
    this->managedConnect(getApp()->accounts->twitch.currentUserChanged, [=] {
        this->refreshPubsub();
    });
    this->refreshPubsub();
    this->userStateChanged.connect([this] {
        this->refreshPubsub();
    });

    // room id loaded -> refresh live status
    this->roomIdChanged.connect([this]() {
        this->refreshPubsub();
        this->refreshTitle();
        this->refreshLiveStatus();
        this->refreshBadges();
        this->refreshCheerEmotes();
        this->refresh7TVChannelEmotes(false);
        this->refreshFFZChannelEmotes(false);
        this->refreshBTTVChannelEmotes(false);
    });

    // timers
    QObject::connect(&this->chattersListTimer_, &QTimer::timeout, [=] {
        this->refreshChatters();
    });
    this->chattersListTimer_.start(5 * 60 * 1000);

    QObject::connect(&this->liveStatusTimer_, &QTimer::timeout, [=] {
        this->refreshLiveStatus();
    });
    this->liveStatusTimer_.start(60 * 1000);

    // debugging
#if 0
    for (int i = 0; i < 1000; i++) {
        this->addMessage(makeSystemMessage("asef"));
    }
#endif
}

void TwitchChannel::initialize()
{
    this->fetchDisplayName();
    this->refreshChatters();
    this->refreshBadges();
}

bool TwitchChannel::isEmpty() const
{
    return this->getName().isEmpty();
}

bool TwitchChannel::canSendMessage() const
{
    return !this->isEmpty();
}

const QString &TwitchChannel::getDisplayName() const
{
    return this->nameOptions.displayName;
}

void TwitchChannel::setDisplayName(const QString &name)
{
    this->nameOptions.displayName = name;
}

const QString &TwitchChannel::getLocalizedName() const
{
    return this->nameOptions.localizedName;
}

void TwitchChannel::setLocalizedName(const QString &name)
{
    this->nameOptions.localizedName = name;
}

void TwitchChannel::refresh7TVChannelEmotes(bool manualRefresh)
{
    SeventvEmotes::loadChannel(
        weakOf<Channel>(this), this->roomId(), this->getName(),
        [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->seventvEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
        },
        manualRefresh);
}

void TwitchChannel::refreshBTTVChannelEmotes(bool manualRefresh)
{
    BttvEmotes::loadChannel(
        weakOf<Channel>(this), this->roomId(), this->getLocalizedName(),
        [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->bttvEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
        },
        manualRefresh);
}

void TwitchChannel::refreshFFZChannelEmotes(bool manualRefresh)
{
    FfzEmotes::loadChannel(
        weakOf<Channel>(this), this->roomId(),
        [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->ffzEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
        },
        [this, weak = weakOf<Channel>(this)](auto &&modBadge) {
            if (auto shared = weak.lock())
            {
                this->ffzCustomModBadge_.set(std::move(modBadge));
            }
        },
        [this, weak = weakOf<Channel>(this)](auto &&vipBadge) {
            if (auto shared = weak.lock())
            {
                this->ffzCustomVipBadge_.set(std::move(vipBadge));
            }
        },
        manualRefresh);
}

void TwitchChannel::addChannelPointReward(const ChannelPointReward &reward)
{
    assertInGuiThread();

    if (!reward.hasParsedSuccessfully)
    {
        return;
    }

    if (!reward.isUserInputRequired)
    {
        MessageBuilder builder;
        TwitchMessageBuilder::appendChannelPointRewardMessage(reward, &builder);
        this->addMessage(builder.release());
        return;
    }

    bool result;
    {
        auto channelPointRewards = this->channelPointRewards_.access();
        result = channelPointRewards->try_emplace(reward.id, reward).second;
    }
    if (result)
    {
        this->channelPointRewardAdded.invoke(reward);
    }
}

bool TwitchChannel::isChannelPointRewardKnown(const QString &rewardId)
{
    const auto &pointRewards = this->channelPointRewards_.accessConst();
    const auto &it = pointRewards->find(rewardId);
    return it != pointRewards->end();
}

boost::optional<ChannelPointReward> TwitchChannel::channelPointReward(
    const QString &rewardId) const
{
    auto rewards = this->channelPointRewards_.accessConst();
    auto it = rewards->find(rewardId);

    if (it == rewards->end())
        return boost::none;
    return it->second;
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto app = getApp();

    if (!app->accounts->twitch.isLoggedIn())
    {
        if (message.isEmpty())
        {
            return;
        }

        const auto linkColor = MessageColor(MessageColor::Link);
        const auto accountsLink = Link(Link::OpenAccountsPage, QString());
        const auto currentUser = getApp()->accounts->twitch.getCurrent();
        const auto expirationText =
            QString("You need to log in to send messages. You can link your "
                    "Twitch account");
        const auto loginPromptText = QString("in the settings.");

        auto builder = MessageBuilder();
        builder.message().flags.set(MessageFlag::System);
        builder.message().flags.set(MessageFlag::DoNotTriggerNotification);

        builder.emplace<TimestampElement>();
        builder.emplace<TextElement>(expirationText, MessageElementFlag::Text,
                                     MessageColor::System);
        builder
            .emplace<TextElement>(loginPromptText, MessageElementFlag::Text,
                                  linkColor)
            ->setLink(accountsLink);

        this->addMessage(builder.release());

        return;
    }

    qCDebug(chatterinoTwitch)
        << "[TwitchChannel" << this->getName() << "] Send message:" << message;

    // Do last message processing
    QString parsedMessage = app->emotes->emojis.replaceShortCodes(message);

    parsedMessage = parsedMessage.trimmed();

    if (parsedMessage.isEmpty())
    {
        return;
    }

    if (!this->hasHighRateLimit())
    {
        if (getSettings()->allowDuplicateMessages)
        {
            if (parsedMessage == this->lastSentMessage_)
            {
                parsedMessage.append(MAGIC_MESSAGE_SUFFIX);
            }
        }
    }

    bool messageSent = false;
    this->sendMessageSignal.invoke(this->getName(), parsedMessage, messageSent);

    if (messageSent)
    {
        qCDebug(chatterinoTwitch) << "sent";
        this->lastSentMessage_ = parsedMessage;
    }
}

bool TwitchChannel::isMod() const
{
    return this->mod_;
}

bool TwitchChannel::isVip() const
{
    return this->vip_;
}

bool TwitchChannel::isStaff() const
{
    return this->staff_;
}

void TwitchChannel::setMod(bool value)
{
    if (this->mod_ != value)
    {
        this->mod_ = value;

        this->userStateChanged.invoke();
    }
}

void TwitchChannel::setVIP(bool value)
{
    if (this->vip_ != value)
    {
        this->vip_ = value;

        this->userStateChanged.invoke();
    }
}

void TwitchChannel::setStaff(bool value)
{
    if (this->staff_ != value)
    {
        this->staff_ = value;

        this->userStateChanged.invoke();
    }
}

bool TwitchChannel::isBroadcaster() const
{
    auto app = getApp();

    return this->getName() == app->accounts->twitch.getCurrent()->getUserName();
}

bool TwitchChannel::hasHighRateLimit() const
{
    return this->isMod() || this->isBroadcaster() || this->isVip();
}

bool TwitchChannel::canReconnect() const
{
    return true;
}

void TwitchChannel::reconnect()
{
    getApp()->twitch.server->connect();
}

QString TwitchChannel::roomId() const
{
    return *this->roomID_.access();
}

void TwitchChannel::setRoomId(const QString &id)
{
    if (*this->roomID_.accessConst() != id)
    {
        *this->roomID_.access() = id;
        this->roomIdChanged.invoke();
        this->loadRecentMessages();
    }
}

SharedAccessGuard<const TwitchChannel::RoomModes>
    TwitchChannel::accessRoomModes() const
{
    return this->roomModes_.accessConst();
}

void TwitchChannel::setRoomModes(const RoomModes &_roomModes)
{
    this->roomModes_ = _roomModes;

    this->roomModesChanged.invoke();
}

bool TwitchChannel::isLive() const
{
    return this->streamStatus_.access()->live;
}

SharedAccessGuard<const TwitchChannel::StreamStatus>
    TwitchChannel::accessStreamStatus() const
{
    return this->streamStatus_.accessConst();
}

const SeventvEmotes &TwitchChannel::globalSeventv() const
{
    return this->globalSeventv_;
}

const BttvEmotes &TwitchChannel::globalBttv() const
{
    return this->globalBttv_;
}

const FfzEmotes &TwitchChannel::globalFfz() const
{
    return this->globalFfz_;
}

boost::optional<EmotePtr> TwitchChannel::seventvEmote(
    const EmoteName &name) const
{
    auto emotes = this->seventvEmotes_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

boost::optional<EmotePtr> TwitchChannel::bttvEmote(const EmoteName &name) const
{
    auto emotes = this->bttvEmotes_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

boost::optional<EmotePtr> TwitchChannel::ffzEmote(const EmoteName &name) const
{
    auto emotes = this->ffzEmotes_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

std::shared_ptr<const EmoteMap> TwitchChannel::seventvEmotes() const
{
    return this->seventvEmotes_.get();
}

std::shared_ptr<const EmoteMap> TwitchChannel::bttvEmotes() const
{
    return this->bttvEmotes_.get();
}

std::shared_ptr<const EmoteMap> TwitchChannel::ffzEmotes() const
{
    return this->ffzEmotes_.get();
}

const QString &TwitchChannel::subscriptionUrl()
{
    return this->subscriptionUrl_;
}

const QString &TwitchChannel::channelUrl()
{
    return this->channelUrl_;
}

const QString &TwitchChannel::popoutPlayerUrl()
{
    return this->popoutPlayerUrl_;
}

int TwitchChannel::chatterCount()
{
    return this->chatterCount_;
}

void TwitchChannel::setLive(bool newLiveStatus)
{
    bool gotNewLiveStatus = false;
    {
        auto guard = this->streamStatus_.access();
        if (guard->live != newLiveStatus)
        {
            gotNewLiveStatus = true;
            if (newLiveStatus)
            {
                if (getApp()->notifications->isChannelNotified(
                        this->getName(), Platform::Twitch))
                {
                    if (Toasts::isEnabled())
                    {
                        getApp()->toasts->sendChannelNotification(
                            this->getName(), Platform::Twitch);
                    }
                    if (getSettings()->notificationPlaySound)
                    {
                        getApp()->notifications->playSound();
                    }
                    if (getSettings()->notificationFlashTaskbar)
                    {
                        getApp()->windows->sendAlert();
                    }
                }
                // Channel live message
                MessageBuilder builder;
                TwitchMessageBuilder::liveSystemMessage(this->getDisplayName(),
                                                        &builder);
                this->addMessage(builder.release());

                // Message in /live channel
                MessageBuilder builder2;
                TwitchMessageBuilder::liveMessage(this->getDisplayName(),
                                                  &builder2);
                getApp()->twitch2->liveChannel->addMessage(builder2.release());

                // Notify on all channels with a ping sound
                if (getSettings()->notificationOnAnyChannel &&
                    !(isInStreamerMode() &&
                      getSettings()->streamerModeSuppressLiveNotifications))
                {
                    getApp()->notifications->playSound();
                }
            }
            else
            {
                // Channel offline message
                MessageBuilder builder;
                TwitchMessageBuilder::offlineSystemMessage(
                    this->getDisplayName(), &builder);
                this->addMessage(builder.release());

                // "delete" old 'CHANNEL is live' message
                LimitedQueueSnapshot<MessagePtr> snapshot =
                    getApp()->twitch2->liveChannel->getMessageSnapshot();
                int snapshotLength = snapshot.size();

                // MSVC hates this code if the parens are not there
                int end = (std::max)(0, snapshotLength - 200);
                auto liveMessageSearchText =
                    QString("%1 is live!").arg(this->getDisplayName());

                for (int i = snapshotLength - 1; i >= end; --i)
                {
                    auto &s = snapshot[i];

                    if (s->messageText == liveMessageSearchText)
                    {
                        s->flags.set(MessageFlag::Disabled);
                        break;
                    }
                }
            }
            guard->live = newLiveStatus;
        }
    }

    if (gotNewLiveStatus)
    {
        this->liveStatusChanged.invoke();
    }
}

void TwitchChannel::refreshTitle()
{
    // timer has never started, proceed and start it
    if (!this->titleRefreshedTimer_.isValid())
    {
        this->titleRefreshedTimer_.start();
    }
    else if (this->roomId().isEmpty() ||
             this->titleRefreshedTimer_.elapsed() < TITLE_REFRESH_PERIOD)
    {
        return;
    }
    this->titleRefreshedTimer_.restart();

    getHelix()->getChannel(
        this->roomId(),
        [this, weak = weakOf<Channel>(this)](HelixChannel channel) {
            ChannelPtr shared = weak.lock();

            if (!shared)
            {
                return;
            }

            {
                auto status = this->streamStatus_.access();
                status->title = channel.title;
            }

            this->liveStatusChanged.invoke();
        },
        [] {
            // failure
        });
}

void TwitchChannel::refreshLiveStatus()
{
    auto roomID = this->roomId();

    if (roomID.isEmpty())
    {
        qCDebug(chatterinoTwitch) << "[TwitchChannel" << this->getName()
                                  << "] Refreshing live status (Missing ID)";
        this->setLive(false);
        return;
    }

    getHelix()->getStreamById(
        roomID,
        [this, weak = weakOf<Channel>(this)](bool live, const auto &stream) {
            ChannelPtr shared = weak.lock();
            if (!shared)
            {
                return;
            }

            this->parseLiveStatus(live, stream);
        },
        [] {
            // failure
        });
}

void TwitchChannel::parseLiveStatus(bool live, const HelixStream &stream)
{
    if (!live)
    {
        this->setLive(false);
        return;
    }

    {
        auto status = this->streamStatus_.access();
        status->viewerCount = stream.viewerCount;
        if (status->gameId != stream.gameId)
        {
            status->gameId = stream.gameId;

            // Resolve game ID to game name
            getHelix()->getGameById(
                stream.gameId,
                [this, weak = weakOf<Channel>(this)](const auto &game) {
                    ChannelPtr shared = weak.lock();
                    if (!shared)
                    {
                        return;
                    }

                    {
                        auto status = this->streamStatus_.access();
                        status->game = game.name;
                    }

                    this->liveStatusChanged.invoke();
                },
                [] {
                    // failure
                });
        }
        status->title = stream.title;
        QDateTime since = QDateTime::fromString(stream.startedAt, Qt::ISODate);
        auto diff = since.secsTo(QDateTime::currentDateTime());
        status->uptime = QString::number(diff / 3600) + "h " +
                         QString::number(diff % 3600 / 60) + "m";

        status->rerun = false;
        status->streamType = stream.type;
    }

    this->setLive(true);

    // Signal all listeners that the stream status has been updated
    this->liveStatusChanged.invoke();
}

void TwitchChannel::loadRecentMessages()
{
    if (!getSettings()->loadTwitchMessageHistoryOnConnect)
    {
        return;
    }

    auto baseURL = Env::get().recentMessagesApiUrl.arg(this->getName());

    auto url = QString("%1?limit=%2")
                   .arg(baseURL)
                   .arg(getSettings()->twitchMessageHistoryLimit);

    NetworkRequest(url)
        .onSuccess([weak = weakOf<Channel>(this)](auto result) -> Outcome {
            auto shared = weak.lock();
            if (!shared)
                return Failure;

            auto messages = parseRecentMessages(result.parseJson(), shared);

            auto &handler = IrcMessageHandler::instance();

            std::vector<MessagePtr> allBuiltMessages;

            for (auto message : messages)
            {
                if (message->tags().contains("rm-received-ts"))
                {
                    QDate msgDate = QDateTime::fromMSecsSinceEpoch(
                                        message->tags()
                                            .value("rm-received-ts")
                                            .toLongLong())
                                        .date();
                    if (msgDate != shared.get()->lastDate_)
                    {
                        shared.get()->lastDate_ = msgDate;
                        auto msg = makeSystemMessage(
                            QLocale().toString(msgDate, QLocale::LongFormat),
                            QTime(0, 0));
                        msg->flags.set(MessageFlag::RecentMessage);
                        allBuiltMessages.emplace_back(msg);
                    }
                }

                for (auto builtMessage :
                     handler.parseMessage(shared.get(), message))
                {
                    builtMessage->flags.set(MessageFlag::RecentMessage);
                    allBuiltMessages.emplace_back(builtMessage);
                }
            }

            postToThread(
                [shared, messages = std::move(allBuiltMessages)]() mutable {
                    shared->addMessagesAtStart(messages);
                });

            return Success;
        })
        .execute();
}

void TwitchChannel::refreshPubsub()
{
    auto roomId = this->roomId();
    if (roomId.isEmpty())
        return;

    auto account = getApp()->accounts->twitch.getCurrent();
    getApp()->twitch2->pubsub->listenToChannelModerationActions(roomId,
                                                                account);
    getApp()->twitch2->pubsub->listenToAutomod(roomId, account);
    getApp()->twitch2->pubsub->listenToChannelPointRewards(roomId, account);
}

void TwitchChannel::refreshChatters()
{
    // setting?
    const auto streamStatus = this->accessStreamStatus();
    const auto viewerCount = static_cast<int>(streamStatus->viewerCount);
    if (getSettings()->onlyFetchChattersForSmallerStreamers)
    {
        if (streamStatus->live &&
            viewerCount > getSettings()->smallStreamerLimit)
        {
            return;
        }
    }

    // get viewer list
    NetworkRequest("https://tmi.twitch.tv/group/user/" + this->getName() +
                   "/chatters")

        .onSuccess(
            [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
                // channel still exists?
                auto shared = weak.lock();
                if (!shared)
                {
                    return Failure;
                }

                auto data = result.parseJson();
                this->chatterCount_ = data.value("chatter_count").toInt();

                auto pair = parseChatters(std::move(data));
                if (pair.first)
                {
                    this->updateOnlineChatters(pair.second);
                }

                return pair.first;
            })
        .execute();
}

void TwitchChannel::fetchDisplayName()
{
    getHelix()->getUserByName(
        this->getName(),
        [weak = weakOf<Channel>(this)](const auto &user) {
            auto shared = weak.lock();
            if (!shared)
                return;
            auto channel = static_cast<TwitchChannel *>(shared.get());
            if (QString::compare(user.displayName, channel->getName(),
                                 Qt::CaseInsensitive) == 0)
            {
                channel->setDisplayName(user.displayName);
                channel->setLocalizedName(user.displayName);
            }
            else
            {
                channel->setLocalizedName(QString("%1(%2)")
                                              .arg(channel->getName())
                                              .arg(user.displayName));
            }
            channel->addRecentChatter(channel->getDisplayName());
            channel->displayNameChanged.invoke();
        },
        [] {});
}

void TwitchChannel::refreshBadges()
{
    auto url = Url{"https://badges.twitch.tv/v1/badges/channels/" +
                   this->roomId() + "/display?language=en"};
    NetworkRequest(url.string)

        .onSuccess([this,
                    weak = weakOf<Channel>(this)](auto result) -> Outcome {
            auto shared = weak.lock();
            if (!shared)
                return Failure;

            auto badgeSets = this->badgeSets_.access();

            auto jsonRoot = result.parseJson();

            auto _ = jsonRoot["badge_sets"].toObject();
            for (auto jsonBadgeSet = _.begin(); jsonBadgeSet != _.end();
                 jsonBadgeSet++)
            {
                auto &versions = (*badgeSets)[jsonBadgeSet.key()];

                auto _set = jsonBadgeSet->toObject()["versions"].toObject();
                for (auto jsonVersion_ = _set.begin();
                     jsonVersion_ != _set.end(); jsonVersion_++)
                {
                    auto jsonVersion = jsonVersion_->toObject();
                    auto emote = std::make_shared<Emote>(Emote{
                        EmoteName{},
                        ImageSet{
                            Image::fromUrl(
                                {jsonVersion["image_url_1x"].toString()}, 1),
                            Image::fromUrl(
                                {jsonVersion["image_url_2x"].toString()}, .5),
                            Image::fromUrl(
                                {jsonVersion["image_url_4x"].toString()}, .25)},
                        Tooltip{jsonVersion["description"].toString()},
                        Url{jsonVersion["clickURL"].toString()}});

                    versions.emplace(jsonVersion_.key(), emote);
                };
            }

            return Success;
        })
        .execute();
}

void TwitchChannel::refreshCheerEmotes()
{
    getHelix()->getCheermotes(
        this->roomId(),
        [this, weak = weakOf<Channel>(this)](
            const std::vector<HelixCheermoteSet> &cheermoteSets) -> Outcome {
            auto shared = weak.lock();
            if (!shared)
            {
                return Failure;
            }

            std::vector<CheerEmoteSet> emoteSets;

            for (const auto &set : cheermoteSets)
            {
                auto cheerEmoteSet = CheerEmoteSet();
                cheerEmoteSet.regex = QRegularExpression(
                    "^" + set.prefix + "([1-9][0-9]*)$",
                    QRegularExpression::CaseInsensitiveOption);

                for (const auto &tier : set.tiers)
                {
                    CheerEmote cheerEmote;

                    cheerEmote.color = QColor(tier.color);
                    cheerEmote.minBits = tier.minBits;
                    cheerEmote.regex = cheerEmoteSet.regex;

                    // TODO(pajlada): We currently hardcode dark here :|
                    // We will continue to do so for now since we haven't had to
                    // solve that anywhere else

                    // Combine the prefix (e.g. BibleThump) with the tier (1, 100 etc.)
                    auto emoteTooltip =
                        set.prefix + tier.id + "<br>Twitch Cheer Emote";
                    cheerEmote.animatedEmote = std::make_shared<Emote>(
                        Emote{EmoteName{"cheer emote"},
                              ImageSet{
                                  tier.darkAnimated.imageURL1x,
                                  tier.darkAnimated.imageURL2x,
                                  tier.darkAnimated.imageURL4x,
                              },
                              Tooltip{emoteTooltip}, Url{}});
                    cheerEmote.staticEmote = std::make_shared<Emote>(
                        Emote{EmoteName{"cheer emote"},
                              ImageSet{
                                  tier.darkStatic.imageURL1x,
                                  tier.darkStatic.imageURL2x,
                                  tier.darkStatic.imageURL4x,
                              },
                              Tooltip{emoteTooltip}, Url{}});

                    cheerEmoteSet.cheerEmotes.emplace_back(
                        std::move(cheerEmote));
                }

                // Sort cheermotes by cost
                std::sort(cheerEmoteSet.cheerEmotes.begin(),
                          cheerEmoteSet.cheerEmotes.end(),
                          [](const auto &lhs, const auto &rhs) {
                              return lhs.minBits > rhs.minBits;
                          });

                emoteSets.emplace_back(std::move(cheerEmoteSet));
            }

            *this->cheerEmoteSets_.access() = std::move(emoteSets);

            return Success;
        },
        [] {
            // Failure
            return Failure;
        });
}

void TwitchChannel::createClip()
{
    if (!this->isLive())
    {
        this->addMessage(makeSystemMessage(
            "Cannot create clip while the channel is offline!"));
        return;
    }

    // timer has never started, proceed and start it
    if (!this->clipCreationTimer_.isValid())
    {
        this->clipCreationTimer_.start();
    }
    else if (this->clipCreationTimer_.elapsed() < CLIP_CREATION_COOLDOWN ||
             this->isClipCreationInProgress)
    {
        return;
    }

    this->addMessage(makeSystemMessage("Creating clip..."));
    this->isClipCreationInProgress = true;

    getHelix()->createClip(
        this->roomId(),
        // successCallback
        [this](const HelixClip &clip) {
            MessageBuilder builder;
            builder.message().flags.set(MessageFlag::System);

            builder.emplace<TimestampElement>();
            // text
            builder.emplace<TextElement>("Clip created!",
                                         MessageElementFlag::Text,
                                         MessageColor::System);
            // clip link
            builder
                .emplace<TextElement>("Copy link to clipboard",
                                      MessageElementFlag::Text,
                                      MessageColor::Link)
                ->setLink(Link(Link::CopyToClipboard, CLIPS_LINK.arg(clip.id)));
            // separator text
            builder.emplace<TextElement>("or", MessageElementFlag::Text,
                                         MessageColor::System);
            // edit link
            builder
                .emplace<TextElement>("edit it in browser.",
                                      MessageElementFlag::Text,
                                      MessageColor::Link)
                ->setLink(Link(Link::Url, clip.editUrl));

            this->addMessage(builder.release());
        },
        // failureCallback
        [this](auto error) {
            MessageBuilder builder;
            builder.message().flags.set(MessageFlag::System);

            builder.emplace<TimestampElement>();

            switch (error)
            {
                case HelixClipError::ClipsDisabled: {
                    builder.emplace<TextElement>(
                        CLIPS_FAILURE_CLIPS_DISABLED_TEXT,
                        MessageElementFlag::Text, MessageColor::System);
                }
                break;

                case HelixClipError::UserNotAuthenticated: {
                    builder.emplace<TextElement>(
                        CLIPS_FAILURE_NOT_AUTHENTICATED_TEXT,
                        MessageElementFlag::Text, MessageColor::System);
                    builder
                        .emplace<TextElement>(LOGIN_PROMPT_TEXT,
                                              MessageElementFlag::Text,
                                              MessageColor::Link)
                        ->setLink(ACCOUNTS_LINK);
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixClipError::Unknown:
                default: {
                    builder.emplace<TextElement>(
                        CLIPS_FAILURE_UNKNOWN_ERROR_TEXT,
                        MessageElementFlag::Text, MessageColor::System);
                }
                break;
            }

            this->addMessage(builder.release());
        },
        // finallyCallback - this will always execute, so clip creation won't ever be stuck
        [this] {
            this->clipCreationTimer_.restart();
            this->isClipCreationInProgress = false;
        });
}

boost::optional<EmotePtr> TwitchChannel::twitchBadge(
    const QString &set, const QString &version) const
{
    auto badgeSets = this->badgeSets_.access();
    auto it = badgeSets->find(set);
    if (it != badgeSets->end())
    {
        auto it2 = it->second.find(version);
        if (it2 != it->second.end())
        {
            return it2->second;
        }
    }
    return boost::none;
}

boost::optional<EmotePtr> TwitchChannel::ffzCustomModBadge() const
{
    return this->ffzCustomModBadge_.get();
}

boost::optional<EmotePtr> TwitchChannel::ffzCustomVipBadge() const
{
    return this->ffzCustomVipBadge_.get();
}

boost::optional<CheerEmote> TwitchChannel::cheerEmote(const QString &string)
{
    auto sets = this->cheerEmoteSets_.access();
    for (const auto &set : *sets)
    {
        auto match = set.regex.match(string);
        if (!match.hasMatch())
        {
            continue;
        }
        QString amount = match.captured(1);
        bool ok = false;
        int bitAmount = amount.toInt(&ok);
        if (!ok)
        {
            qCDebug(chatterinoTwitch)
                << "Error parsing bit amount in cheerEmote";
        }
        for (const auto &emote : set.cheerEmotes)
        {
            if (bitAmount >= emote.minBits)
            {
                return emote;
            }
        }
    }
    return boost::none;
}

}  // namespace chatterino
