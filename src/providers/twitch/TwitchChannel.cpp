#include "providers/twitch/TwitchChannel.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/TwitchParseCheerEmotes.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"
#include "widgets/Window.hpp"

#include <IrcConnection>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QTimer>

namespace chatterino {
namespace {
    constexpr char MAGIC_MESSAGE_SUFFIX[] = u8" \U000E0000";

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
            messages.emplace_back(
                Communi::IrcMessage::fromData(content, nullptr));
        }

        return messages;
    }
    std::pair<Outcome, UsernameSet> parseChatters(const QJsonObject &jsonRoot)
    {
        static QStringList categories = {"broadcaster", "vips",   "moderators",
                                         "staff",       "admins", "global_mods",
                                         "viewers"};

        auto usernames = UsernameSet();

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

TwitchChannel::TwitchChannel(const QString &name,
                             TwitchBadges &globalTwitchBadges, BttvEmotes &bttv,
                             FfzEmotes &ffz)
    : Channel(name, Channel::Type::Twitch)
    , ChannelChatters(*static_cast<Channel *>(this))
    , subscriptionUrl_("https://www.twitch.tv/subs/" + name)
    , channelUrl_("https://twitch.tv/" + name)
    , popoutPlayerUrl_("https://player.twitch.tv/?channel=" + name)
    , globalTwitchBadges_(globalTwitchBadges)
    , globalBttv_(bttv)
    , globalFfz_(ffz)
    , bttvEmotes_(std::make_shared<EmoteMap>())
    , ffzEmotes_(std::make_shared<EmoteMap>())
    , mod_(false)
{
    log("[TwitchChannel:{}] Opened", name);

    this->liveStatusChanged.connect([this]() {
        if (this->isLive() == 1)
        {
        }
    });

    this->managedConnect(getApp()->accounts->twitch.currentUserChanged,
                         [=] { this->setMod(false); });

    // pubsub
    this->managedConnect(getApp()->accounts->twitch.currentUserChanged,
                         [=] { this->refreshPubsub(); });
    this->refreshPubsub();
    this->userStateChanged.connect([this] { this->refreshPubsub(); });

    // room id loaded -> refresh live status
    this->roomIdChanged.connect([this]() {
        this->refreshPubsub();
        this->refreshLiveStatus();
        this->refreshBadges();
        this->refreshCheerEmotes();
        this->refreshFFZChannelEmotes();
        this->refreshBTTVChannelEmotes();
    });

    // timers
    QObject::connect(&this->chattersListTimer_, &QTimer::timeout,
                     [=] { this->refreshChatters(); });
    this->chattersListTimer_.start(5 * 60 * 1000);

    QObject::connect(&this->liveStatusTimer_, &QTimer::timeout,
                     [=] { this->refreshLiveStatus(); });
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

void TwitchChannel::refreshBTTVChannelEmotes()
{
    BttvEmotes::loadChannel(
        this->roomId(), [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->bttvEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
        });
}

void TwitchChannel::refreshFFZChannelEmotes()
{
    FfzEmotes::loadChannel(
        this->roomId(),
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
        });
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto app = getApp();

    if (!app->accounts->twitch.isLoggedIn())
    {
        // XXX: It would be nice if we could add a link here somehow that opened
        // the "account manager" dialog
        this->addMessage(
            makeSystemMessage("You need to log in to send messages. You can "
                              "link your Twitch account in the settings."));
        return;
    }

    log("[TwitchChannel:{}] Send message: {}", this->getName(), message);

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
        qDebug() << "sent";
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

AccessGuard<const TwitchChannel::RoomModes> TwitchChannel::accessRoomModes()
    const
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

AccessGuard<const TwitchChannel::StreamStatus>
    TwitchChannel::accessStreamStatus() const
{
    return this->streamStatus_.accessConst();
}

const TwitchBadges &TwitchChannel::globalTwitchBadges() const
{
    return this->globalTwitchBadges_;
}

const BttvEmotes &TwitchChannel::globalBttv() const
{
    return this->globalBttv_;
}

const FfzEmotes &TwitchChannel::globalFfz() const
{
    return this->globalFfz_;
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
                auto live =
                    makeSystemMessage(this->getDisplayName() + " is live");
                this->addMessage(live);
            }
            else
            {
                auto offline =
                    makeSystemMessage(this->getDisplayName() + " is offline");
                this->addMessage(offline);
            }
            guard->live = newLiveStatus;
        }
    }

    if (gotNewLiveStatus)
    {
        this->liveStatusChanged.invoke();
    }
}

void TwitchChannel::refreshTitle(){
    auto roomID= this->roomId();
    if(roomID.isEmpty()){
        return;
    }

    this->streamStatus_.access()->title = "hello";

    QString url("https://api.twitch.tv/kraken/channels/"+roomID);
    NetworkRequest::twitchRequest(url)
        .onSuccess(
            [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
                ChannelPtr shared = weak.lock();
                if (!shared)
                    return Failure;

                const auto document = result.parseRapidJson();

                if(!document.HasMember("status")){
                    return Failure;
                }

                {
                    auto status = this->streamStatus_.access();
                    status->title = document["status"].GetString();
                }

                this->liveStatusChanged.invoke();
                return Success;
            })
        .execute();
}

void TwitchChannel::refreshLiveStatus()
{
    auto roomID = this->roomId();

    if (roomID.isEmpty())
    {
        log("[TwitchChannel:{}] Refreshing live status (Missing ID)",
            this->getName());
        this->setLive(false);
        return;
    }

    QString url("https://api.twitch.tv/kraken/streams/" + roomID);

    //    auto request = makeGetStreamRequest(roomID, QThread::currentThread());
    NetworkRequest::twitchRequest(url)

        .onSuccess(
            [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
                ChannelPtr shared = weak.lock();
                if (!shared || !this->parseLiveStatus(result.parseRapidJson()))
                {
                    this->refreshTitle();
                    return Failure;
                }

                return Success;
            })
        .execute();
}

Outcome TwitchChannel::parseLiveStatus(const rapidjson::Document &document)
{
    if (!document.IsObject())
    {
        log("[TwitchChannel:refreshLiveStatus] root is not an object");
        return Failure;
    }

    if (!document.HasMember("stream"))
    {
        log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
        return Failure;
    }

    const auto &stream = document["stream"];

    if (!stream.IsObject())
    {
        // Stream is offline (stream is most likely null)
        this->setLive(false);
        return Failure;
    }

    if (!stream.HasMember("viewers") || !stream.HasMember("game") ||
        !stream.HasMember("channel") || !stream.HasMember("created_at"))
    {
        log("[TwitchChannel:refreshLiveStatus] Missing members in stream");
        this->setLive(false);
        return Failure;
    }

    const rapidjson::Value &streamChannel = stream["channel"];

    if (!streamChannel.IsObject() || !streamChannel.HasMember("status"))
    {
        log("[TwitchChannel:refreshLiveStatus] Missing member \"status\" in "
            "channel");
        return Failure;
    }

    // Stream is live

    {
        auto status = this->streamStatus_.access();
        status->viewerCount = stream["viewers"].GetUint();
        status->title = streamChannel["status"].GetString();
        status->game = stream["game"].GetString();
        QDateTime since = QDateTime::fromString(
            stream["created_at"].GetString(), Qt::ISODate);
        auto diff = since.secsTo(QDateTime::currentDateTime());
        status->uptime = QString::number(diff / 3600) + "h " +
                         QString::number(diff % 3600 / 60) + "m";

        status->rerun = false;
        if (stream.HasMember("stream_type"))
        {
            status->streamType = stream["stream_type"].GetString();
        }
        else
        {
            status->streamType = QString();
        }

        if (stream.HasMember("broadcast_platform"))
        {
            const auto &broadcastPlatformValue = stream["broadcast_platform"];

            if (broadcastPlatformValue.IsString())
            {
                const char *broadcastPlatform =
                    stream["broadcast_platform"].GetString();
                if (strcmp(broadcastPlatform, "rerun") == 0)
                {
                    status->rerun = true;
                }
            }
        }
    }
    setLive(true);
    // Signal all listeners that the stream status has been updated
    this->liveStatusChanged.invoke();

    return Success;
}

void TwitchChannel::loadRecentMessages()
{
    if (!getSettings()->loadTwitchMessageHistoryOnConnect)
    {
        return;
    }

    NetworkRequest(Env::get().recentMessagesApiUrl.arg(this->getName()))
        .concurrent()
        .onSuccess([weak = weakOf<Channel>(this)](auto result) -> Outcome {
            auto shared = weak.lock();
            if (!shared)
                return Failure;

            auto messages = parseRecentMessages(result.parseJson(), shared);

            auto &handler = IrcMessageHandler::instance();

            std::vector<MessagePtr> allBuiltMessages;

            for (auto message : messages)
            {
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
    // listen to moderation actions
    if (!this->hasModRights())
        return;
    auto roomId = this->roomId();
    if (roomId.isEmpty())
        return;

    auto account = getApp()->accounts->twitch.getCurrent();
    getApp()->twitch2->pubsub->listenToChannelModerationActions(roomId,
                                                                account);
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
                    return Failure;

                auto pair = parseChatters(result.parseJson());
                if (pair.first)
                {
                    this->setChatters(std::move(pair.second));
                }

                return pair.first;
            })
        .execute();
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
    QString url("https://api.twitch.tv/kraken/bits/actions?channel_id=" +
                this->roomId());
    NetworkRequest::twitchRequest(url)
        .onSuccess([this,
                    weak = weakOf<Channel>(this)](auto result) -> Outcome {
            auto cheerEmoteSets = ParseCheermoteSets(result.parseRapidJson());
            std::vector<CheerEmoteSet> emoteSets;

            for (auto &set : cheerEmoteSets)
            {
                auto cheerEmoteSet = CheerEmoteSet();
                cheerEmoteSet.regex = QRegularExpression(
                    "^" + set.prefix + "([1-9][0-9]*)$",
                    QRegularExpression::CaseInsensitiveOption);

                for (auto &tier : set.tiers)
                {
                    CheerEmote cheerEmote;

                    cheerEmote.color = QColor(tier.color);
                    cheerEmote.minBits = tier.minBits;

                    // TODO(pajlada): We currently hardcode dark here :|
                    // We will continue to do so for now since we haven't had to
                    // solve that anywhere else

                    cheerEmote.animatedEmote = std::make_shared<Emote>(
                        Emote{EmoteName{"cheer emote"},
                              ImageSet{
                                  tier.images["dark"]["animated"]["1"],
                                  tier.images["dark"]["animated"]["2"],
                                  tier.images["dark"]["animated"]["4"],
                              },
                              Tooltip{}, Url{}});
                    cheerEmote.staticEmote = std::make_shared<Emote>(
                        Emote{EmoteName{"cheer emote"},
                              ImageSet{
                                  tier.images["dark"]["static"]["1"],
                                  tier.images["dark"]["static"]["2"],
                                  tier.images["dark"]["static"]["4"],
                              },
                              Tooltip{}, Url{}});

                    cheerEmoteSet.cheerEmotes.emplace_back(cheerEmote);
                }

                std::sort(cheerEmoteSet.cheerEmotes.begin(),
                          cheerEmoteSet.cheerEmotes.end(),
                          [](const auto &lhs, const auto &rhs) {
                              return lhs.minBits > rhs.minBits;
                          });

                emoteSets.emplace_back(cheerEmoteSet);
            }
            *this->cheerEmoteSets_.access() = std::move(emoteSets);

            return Success;
        })
        .execute();
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
            log("Error parsing bit amount in cheerEmote");
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
