#include "providers/twitch/TwitchChannel.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/bttv/LoadBttvChannelEmote.hpp"
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
    auto parseRecentMessages(const QJsonObject &jsonRoot, ChannelPtr channel)
    {
        QJsonArray jsonMessages = jsonRoot.value("messages").toArray();
        std::vector<MessagePtr> messages;

        if (jsonMessages.empty())
            return messages;

        for (const auto jsonMessage : jsonMessages)
        {
            auto content = jsonMessage.toString().toUtf8();
            // passing nullptr as the channel makes the message invalid but we
            // don't check for that anyways
            auto message = Communi::IrcMessage::fromData(content, nullptr);
            auto privMsg = dynamic_cast<Communi::IrcPrivateMessage *>(message);
            assert(privMsg);

            MessageParseArgs args;
            TwitchMessageBuilder builder(channel.get(), privMsg, args);
            if (getSettings()->greyOutHistoricMessages)
                builder.message().flags.set(MessageFlag::Disabled);

            if (!builder.isIgnored())
                messages.push_back(builder.build());
        }

        return messages;
    }
    std::pair<Outcome, UsernameSet> parseChatters(const QJsonObject &jsonRoot)
    {
        static QStringList categories = {"moderators", "staff", "admins",
                                         "global_mods", "viewers"};

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
    , subscriptionUrl_("https://www.twitch.tv/subs/" + name)
    , channelUrl_("https://twitch.tv/" + name)
    , popoutPlayerUrl_("https://player.twitch.tv/?channel=" + name)
    , globalTwitchBadges_(globalTwitchBadges)
    , globalBttv_(bttv)
    , globalFfz_(ffz)
    , bttvEmotes_(std::make_shared<EmoteMap>())
    , ffzEmotes_(std::make_shared<EmoteMap>())
    , ffzCustomModBadge_(name)
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
    });

    // timers
    QObject::connect(&this->chattersListTimer_, &QTimer::timeout,
                     [=] { this->refreshChatters(); });
    this->chattersListTimer_.start(5 * 60 * 1000);

    QObject::connect(&this->liveStatusTimer_, &QTimer::timeout,
                     [=] { this->refreshLiveStatus(); });
    this->liveStatusTimer_.start(60 * 1000);

    // --
    this->messageSuffix_.append(' ');
    this->messageSuffix_.append(QChar(0x206D));

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
    this->refreshChannelEmotes();
    this->refreshBadges();
    this->ffzCustomModBadge_.loadCustomModBadge();
}

bool TwitchChannel::isEmpty() const
{
    return this->getName().isEmpty();
}

bool TwitchChannel::canSendMessage() const
{
    return !this->isEmpty();
}

void TwitchChannel::refreshChannelEmotes()
{
    BttvEmotes::loadChannel(
        this->getName(), [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->bttvEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
        });
    FfzEmotes::loadChannel(
        this->getName(), [this, weak = weakOf<Channel>(this)](auto &&emoteMap) {
            if (auto shared = weak.lock())
                this->ffzEmotes_.set(
                    std::make_shared<EmoteMap>(std::move(emoteMap)));
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

    if (!this->hasModRights())
    {
        if (getSettings()->allowDuplicateMessages)
        {
            if (parsedMessage == this->lastSentMessage_)
            {
                parsedMessage.append(this->messageSuffix_);
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

void TwitchChannel::setMod(bool value)
{
    if (this->mod_ != value)
    {
        this->mod_ = value;

        this->userStateChanged.invoke();
    }
}

bool TwitchChannel::isBroadcaster() const
{
    auto app = getApp();

    return this->getName() == app->accounts->twitch.getCurrent()->getUserName();
}

void TwitchChannel::addRecentChatter(const MessagePtr &message)
{
    this->chatters_.access()->insert(message->displayName);
}

void TwitchChannel::addJoinedUser(const QString &user)
{
    auto app = getApp();
    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !getSettings()->showJoins.getValue())
    {
        return;
    }

    auto joinedUsers = this->joinedUsers_.access();
    joinedUsers->append(user);

    if (!this->joinedUsersMergeQueued_)
    {
        this->joinedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto joinedUsers = this->joinedUsers_.access();

            MessageBuilder builder(systemMessage,
                                   "Users joined: " + joinedUsers->join(", "));
            builder->flags.set(MessageFlag::Collapsed);
            joinedUsers->clear();
            this->addMessage(builder.release());
            this->joinedUsersMergeQueued_ = false;
        });
    }
}

void TwitchChannel::addPartedUser(const QString &user)
{
    auto app = getApp();

    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !getSettings()->showJoins.getValue())
    {
        return;
    }

    auto partedUsers = this->partedUsers_.access();
    partedUsers->append(user);

    if (!this->partedUsersMergeQueued_)
    {
        this->partedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto partedUsers = this->partedUsers_.access();

            MessageBuilder builder(systemMessage,
                                   "Users parted: " + partedUsers->join(", "));
            builder->flags.set(MessageFlag::Collapsed);
            this->addMessage(builder.release());
            partedUsers->clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
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

AccessGuard<const UsernameSet> TwitchChannel::accessChatters() const
{
    return this->chatters_.accessConst();
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
                auto live = makeSystemMessage(this->getName() + " is live");
                this->addMessage(live);
            }
            else
            {
                auto offline =
                    makeSystemMessage(this->getName() + " is offline");
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
    auto request = NetworkRequest::twitchRequest(url);
    request.setCaller(QThread::currentThread());

    request.onSuccess(
        [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
            ChannelPtr shared = weak.lock();
            if (!shared)
                return Failure;

            return this->parseLiveStatus(result.parseRapidJson());
        });

    request.execute();
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
        status->game = stream["game"].GetString();
        status->title = streamChannel["status"].GetString();
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
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" +
        getDefaultClientID();

    NetworkRequest request(genericURL.arg(this->roomId()));
    request.makeAuthorizedV5(getDefaultClientID());
    request.setCaller(QThread::currentThread());
    // can't be concurrent right now due to SignalVector
    //    request.setExecuteConcurrently(true);

    request.onSuccess([weak = weakOf<Channel>(this)](auto result) -> Outcome {
        auto shared = weak.lock();
        if (!shared)
            return Failure;

        auto messages = parseRecentMessages(result.parseJson(), shared);

        shared->addMessagesAtStart(messages);

        return Success;
    });

    request.execute();
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

    if (getSettings()->onlyFetchChattersForSmallerStreamers)
    {
        if (streamStatus->live &&
            streamStatus->viewerCount > getSettings()->smallStreamerLimit)
        {
            return;
        }
    }

    // get viewer list
    NetworkRequest request("https://tmi.twitch.tv/group/user/" +
                           this->getName() + "/chatters");

    request.setCaller(QThread::currentThread());
    request.onSuccess(
        [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
            // channel still exists?
            auto shared = weak.lock();
            if (!shared)
                return Failure;

            auto pair = parseChatters(result.parseJson());
            if (pair.first)
            {
                *this->chatters_.access() = std::move(pair.second);
            }

            return pair.first;
        });

    request.execute();
}

void TwitchChannel::refreshBadges()
{
    auto url = Url{"https://badges.twitch.tv/v1/badges/channels/" +
                   this->roomId() + "/display?language=en"};
    NetworkRequest req(url.string);
    req.setCaller(QThread::currentThread());

    req.onSuccess([this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
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

            auto _ = jsonBadgeSet->toObject()["versions"].toObject();
            for (auto jsonVersion_ = _.begin(); jsonVersion_ != _.end();
                 jsonVersion_++)
            {
                auto jsonVersion = jsonVersion_->toObject();
                auto emote = std::make_shared<Emote>(Emote{
                    EmoteName{},
                    ImageSet{
                        Image::fromUrl({jsonVersion["image_url_1x"].toString()},
                                       1),
                        Image::fromUrl({jsonVersion["image_url_2x"].toString()},
                                       .5),
                        Image::fromUrl({jsonVersion["image_url_4x"].toString()},
                                       .25)},
                    Tooltip{jsonVersion["description"].toString()},
                    Url{jsonVersion["clickURL"].toString()}});

                versions.emplace(jsonVersion_.key(), emote);
            };
        }

        return Success;
    });

    req.execute();
}

void TwitchChannel::refreshCheerEmotes()
{
    /*auto url = Url{"https://api.twitch.tv/kraken/bits/actions?channel_id=" +
                   this->getRoomId()};
    auto request = NetworkRequest::twitchRequest(url.string);
    request.setCaller(QThread::currentThread());

    request.onSuccess(
        [this, weak = weakOf<Channel>(this)](auto result) -> Outcome {
            auto cheerEmoteSets = ParseCheermoteSets(result.parseRapidJson());
            std::vector<CheerEmoteSet> emoteSets;

            for (auto &set : cheerEmoteSets) {
                auto cheerEmoteSet = CheerEmoteSet();
                cheerEmoteSet.regex = QRegularExpression(
                    "^" + set.prefix.toLower() + "([1-9][0-9]*)$");

                for (auto &tier : set.tiers) {
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
                              return lhs.minBits < rhs.minBits;  //
                          });

                emoteSets.emplace_back(cheerEmoteSet);
            }
            *this->cheerEmoteSets_.access() = std::move(emoteSets);

            return Success;
        });

    request.execute();
    */
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
    if (auto badge = this->ffzCustomModBadge_.badge())
        return badge;

    return boost::none;
}

}  // namespace chatterino
