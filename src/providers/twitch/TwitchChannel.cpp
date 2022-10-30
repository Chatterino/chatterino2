#include "providers/twitch/TwitchChannel.hpp"

#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/Message.hpp"
#include "providers/RecentMessagesApi.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"
#include "util/QStringHash.hpp"
#include "widgets/Window.hpp"

#include <rapidjson/document.h>
#include <IrcConnection>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QTimer>

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

TwitchChannel::TwitchChannel(const QString &name)
    : Channel(name, Channel::Type::Twitch)
    , ChannelChatters(*static_cast<Channel *>(this))
    , nameOptions{name, name}
    , subscriptionUrl_("https://www.twitch.tv/subs/" + name)
    , channelUrl_("https://twitch.tv/" + name)
    , popoutPlayerUrl_("https://player.twitch.tv/?parent=twitch.tv&channel=" +
                       name)
    , bttvEmotes_(std::make_shared<EmoteMap>())
    , ffzEmotes_(std::make_shared<EmoteMap>())
    , seventvEmotes_(std::make_shared<EmoteMap>())
    , mod_(false)
{
    qCDebug(chatterinoTwitch) << "[TwitchChannel" << name << "] Opened";

    this->bSignals_.emplace_back(
        getApp()->accounts->twitch.currentUserChanged.connect([=] {
            this->setMod(false);
            this->refreshPubSub();
        }));

    this->refreshPubSub();
    this->userStateChanged.connect([this] {
        this->refreshPubSub();
    });

    // room id loaded -> refresh live status
    this->roomIdChanged.connect([this]() {
        this->refreshPubSub();
        this->refreshTitle();
        this->refreshLiveStatus();
        this->refreshBadges();
        this->refreshCheerEmotes();
        this->refreshFFZChannelEmotes(false);
        this->refreshBTTVChannelEmotes(false);
        this->refreshSevenTVChannelEmotes(false);
    });

    this->connected.connect([this]() {
        if (this->roomId().isEmpty())
        {
            // If we get a reconnected event when the room id is not set, we
            // just connected for the first time. After receiving the first
            // message from a channel, setRoomId is called and further
            // invocations of this event will load recent messages.
            return;
        }

        this->loadRecentMessagesReconnect();
    });

    this->messageRemovedFromStart.connect([this](MessagePtr &msg) {
        if (msg->replyThread)
        {
            if (msg->replyThread->liveCount(msg) == 0)
            {
                this->threads_.erase(msg->replyThread->rootId());
            }
        }
    });

    // timers
    QObject::connect(&this->chattersListTimer_, &QTimer::timeout, [=] {
        this->refreshChatters();
    });
    this->chattersListTimer_.start(5 * 60 * 1000);

    QObject::connect(&this->threadClearTimer_, &QTimer::timeout, [=] {
        // We periodically check for any dangling reply threads that missed
        // being cleaned up on messageRemovedFromStart. This could occur if
        // some other part of the program, like a user card, held a reference
        // to the message.
        //
        // It seems difficult to actually replicate a situation where things
        // are actually cleaned up, but I've verified that cleanups DO happen.
        this->cleanUpReplyThreads();
    });
    this->threadClearTimer_.start(5 * 60 * 1000);

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

void TwitchChannel::refreshBTTVChannelEmotes(bool manualRefresh)
{
    if (!Settings::instance().enableBTTVChannelEmotes)
    {
        this->bttvEmotes_.set(EMPTY_EMOTE_MAP);
        return;
    }

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
    if (!Settings::instance().enableFFZChannelEmotes)
    {
        this->ffzEmotes_.set(EMPTY_EMOTE_MAP);
        return;
    }

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

void TwitchChannel::refreshSevenTVChannelEmotes(bool manualRefresh)
{
    if (!Settings::instance().enableSevenTVChannelEmotes)
    {
        this->seventvEmotes_.set(EMPTY_EMOTE_MAP);
        return;
    }

    SeventvEmotes::loadChannelEmotes(
        weakOf<Channel>(this), this->roomId(),
        [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
            {
                this->seventvEmotes_.set(std::make_shared<EmoteMap>(
                    std::forward<decltype(emoteMap)>(emoteMap)));
            }
        },
        manualRefresh);
}

void TwitchChannel::addChannelPointReward(const ChannelPointReward &reward)
{
    assertInGuiThread();

    if (!reward.isUserInputRequired)
    {
        MessageBuilder builder;
        TwitchMessageBuilder::appendChannelPointRewardMessage(
            reward, &builder, this->isMod(), this->isBroadcaster());
        this->addMessage(builder.release());
        return;
    }

    bool result = false;
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

void TwitchChannel::showLoginMessage()
{
    const auto linkColor = MessageColor(MessageColor::Link);
    const auto accountsLink = Link(Link::OpenAccountsPage, QString());
    const auto currentUser = getApp()->accounts->twitch.getCurrent();
    const auto expirationText =
        QStringLiteral("You need to log in to send messages. You can link your "
                       "Twitch account");
    const auto loginPromptText = QStringLiteral("in the settings.");

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
}

QString TwitchChannel::prepareMessage(const QString &message) const
{
    auto app = getApp();
    QString parsedMessage = app->emotes->emojis.replaceShortCodes(message);

    // This is to make sure that combined emoji go through properly, see
    // https://github.com/Chatterino/chatterino2/issues/3384 and
    // https://mm2pl.github.io/emoji_rfc.pdf for more details
    parsedMessage.replace(ZERO_WIDTH_JOINER, ESCAPE_TAG);
    parsedMessage = parsedMessage.simplified();

    if (parsedMessage.isEmpty())
    {
        return "";
    }

    if (!this->hasHighRateLimit())
    {
        if (getSettings()->allowDuplicateMessages)
        {
            if (parsedMessage == this->lastSentMessage_)
            {
                auto spaceIndex = parsedMessage.indexOf(' ');
                // If the message starts with either '/' or '.' Twitch will treat it as a command, omitting
                // first space and only rest of the arguments treated as actual message content
                // In cases when user sends a message like ". .a b" first character and first space are omitted as well
                bool ignoreFirstSpace =
                    parsedMessage.at(0) == '/' || parsedMessage.at(0) == '.';
                if (ignoreFirstSpace)
                {
                    spaceIndex = parsedMessage.indexOf(' ', spaceIndex + 1);
                }

                if (spaceIndex == -1)
                {
                    // no spaces found, fall back to old magic character
                    parsedMessage.append(MAGIC_MESSAGE_SUFFIX);
                }
                else
                {
                    // replace the space we found in spaceIndex with two spaces
                    parsedMessage.replace(spaceIndex, 1, "  ");
                }
            }
        }
    }

    return parsedMessage;
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto app = getApp();
    if (!app->accounts->twitch.isLoggedIn())
    {
        if (!message.isEmpty())
        {
            this->showLoginMessage();
        }

        return;
    }

    qCDebug(chatterinoTwitch)
        << "[TwitchChannel" << this->getName() << "] Send message:" << message;

    // Do last message processing
    QString parsedMessage = this->prepareMessage(message);
    if (parsedMessage.isEmpty())
    {
        return;
    }

    bool messageSent = false;
    this->sendMessageSignal.invoke(this->getName(), parsedMessage, messageSent);

    if (messageSent)
    {
        qCDebug(chatterinoTwitch) << "sent";
        this->lastSentMessage_ = parsedMessage;
    }
}

void TwitchChannel::sendReply(const QString &message, const QString &replyId)
{
    auto app = getApp();
    if (!app->accounts->twitch.isLoggedIn())
    {
        if (!message.isEmpty())
        {
            this->showLoginMessage();
        }

        return;
    }

    qCDebug(chatterinoTwitch) << "[TwitchChannel" << this->getName()
                              << "] Send reply message:" << message;

    // Do last message processing
    QString parsedMessage = this->prepareMessage(message);
    if (parsedMessage.isEmpty())
    {
        return;
    }

    bool messageSent = false;
    this->sendReplySignal.invoke(this->getName(), parsedMessage, replyId,
                                 messageSent);

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
    getApp()->twitch->connect();
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

boost::optional<EmotePtr> TwitchChannel::seventvEmote(
    const EmoteName &name) const
{
    auto emotes = this->seventvEmotes_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
    {
        return boost::none;
    }
    return it->second;
}

std::shared_ptr<const EmoteMap> TwitchChannel::bttvEmotes() const
{
    return this->bttvEmotes_.get();
}

std::shared_ptr<const EmoteMap> TwitchChannel::ffzEmotes() const
{
    return this->ffzEmotes_.get();
}

std::shared_ptr<const EmoteMap> TwitchChannel::seventvEmotes() const
{
    return this->seventvEmotes_.get();
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
                getApp()->twitch->liveChannel->addMessage(builder2.release());

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
                    getApp()->twitch->liveChannel->getMessageSnapshot();
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
        },
        [] {
            // finally
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
        status->gameId = stream.gameId;
        status->game = stream.gameName;
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

    if (this->loadingRecentMessages_.test_and_set())
    {
        return;  // already loading
    }

    auto weak = weakOf<Channel>(this);
    RecentMessagesApi::loadRecentMessages(
        this->getName(), weak,
        [weak](const auto &messages) {
            auto shared = weak.lock();
            if (!shared)
                return;

            auto tc = dynamic_cast<TwitchChannel *>(shared.get());
            if (!tc)
                return;

            tc->addMessagesAtStart(messages);
            tc->loadingRecentMessages_.clear();
        },
        [weak]() {
            auto shared = weak.lock();
            if (!shared)
                return;

            auto tc = dynamic_cast<TwitchChannel *>(shared.get());
            if (!tc)
                return;

            tc->loadingRecentMessages_.clear();
        });
}

void TwitchChannel::loadRecentMessagesReconnect()
{
    if (!getSettings()->loadTwitchMessageHistoryOnConnect)
    {
        return;
    }

    if (this->loadingRecentMessages_.test_and_set())
    {
        return;  // already loading
    }

    auto weak = weakOf<Channel>(this);
    RecentMessagesApi::loadRecentMessages(
        this->getName(), weak,
        [weak](const auto &messages) {
            auto shared = weak.lock();
            if (!shared)
                return;

            auto tc = dynamic_cast<TwitchChannel *>(shared.get());
            if (!tc)
                return;

            tc->fillInMissingMessages(messages);
            tc->loadingRecentMessages_.clear();
        },
        [weak]() {
            auto shared = weak.lock();
            if (!shared)
                return;

            auto tc = dynamic_cast<TwitchChannel *>(shared.get());
            if (!tc)
                return;

            tc->loadingRecentMessages_.clear();
        });
}

void TwitchChannel::refreshPubSub()
{
    auto roomId = this->roomId();
    if (roomId.isEmpty())
    {
        return;
    }

    auto currentAccount = getApp()->accounts->twitch.getCurrent();

    getApp()->twitch->pubsub->setAccount(currentAccount);

    getApp()->twitch->pubsub->listenToChannelModerationActions(roomId);
    getApp()->twitch->pubsub->listenToAutomod(roomId);
    getApp()->twitch->pubsub->listenToChannelPointRewards(roomId);
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

void TwitchChannel::addReplyThread(const std::shared_ptr<MessageThread> &thread)
{
    this->threads_[thread->rootId()] = thread;
}

const std::unordered_map<QString, std::weak_ptr<MessageThread>>
    &TwitchChannel::threads() const
{
    return this->threads_;
}

void TwitchChannel::cleanUpReplyThreads()
{
    for (auto it = this->threads_.begin(), last = this->threads_.end();
         it != last;)
    {
        bool doErase = true;
        if (auto thread = it->second.lock())
        {
            doErase = thread->liveCount() == 0;
        }

        if (doErase)
        {
            it = this->threads_.erase(it);
        }
        else
        {
            ++it;
        }
    }
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
            QString text(
                "Clip created! Copy link to clipboard or edit it in browser.");
            builder.message().messageText = text;
            builder.message().searchText = text;
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
            QString text;
            builder.message().flags.set(MessageFlag::System);

            builder.emplace<TimestampElement>();

            switch (error)
            {
                case HelixClipError::ClipsDisabled: {
                    builder.emplace<TextElement>(
                        CLIPS_FAILURE_CLIPS_DISABLED_TEXT,
                        MessageElementFlag::Text, MessageColor::System);
                    text = CLIPS_FAILURE_CLIPS_DISABLED_TEXT;
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
                    text = QString("%1 %2").arg(
                        CLIPS_FAILURE_NOT_AUTHENTICATED_TEXT,
                        LOGIN_PROMPT_TEXT);
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixClipError::Unknown:
                default: {
                    builder.emplace<TextElement>(
                        CLIPS_FAILURE_UNKNOWN_ERROR_TEXT,
                        MessageElementFlag::Text, MessageColor::System);
                    text = CLIPS_FAILURE_UNKNOWN_ERROR_TEXT;
                }
                break;
            }

            builder.message().messageText = text;
            builder.message().searchText = text;

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
