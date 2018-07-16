#include "providers/twitch/TwitchChannel.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "util/PostToThread.hpp"

#include <IrcConnection>
#include <QJsonArray>
#include <QThread>
#include <QTimer>

namespace chatterino {

TwitchChannel::TwitchChannel(const QString &channelName)
    : Channel(channelName, Channel::Type::Twitch)
    , bttvEmotes_(new EmoteMap)
    , ffzEmotes_(new EmoteMap)
    , subscriptionUrl_("https://www.twitch.tv/subs/" + name)
    , channelUrl_("https://twitch.tv/" + name)
    , popoutPlayerUrl_("https://player.twitch.tv/?channel=" + name)
    , mod_(false)
{
    Log("[TwitchChannel:{}] Opened", this->name);

    this->refreshChannelEmotes();
    // this->refreshViewerList();

    this->managedConnect(getApp()->accounts->twitch.currentUserChanged,
                         [=] { this->setMod(false); });

    // pubsub
    this->userStateChanged.connect([=] { this->refreshPubsub(); });
    this->roomIdChanged.connect([=] { this->refreshPubsub(); });
    this->managedConnect(getApp()->accounts->twitch.currentUserChanged,
                         [=] { this->refreshPubsub(); });
    this->refreshPubsub();

    // room id loaded -> refresh live status
    this->roomIdChanged.connect([this]() { this->refreshLiveStatus(); });

    // timers
    QObject::connect(&this->chattersListTimer_, &QTimer::timeout,
                     [=] { this->refreshViewerList(); });
    this->chattersListTimer_.start(5 * 60 * 1000);

    QObject::connect(&this->liveStatusTimer_, &QTimer::timeout, [=] { this->refreshLiveStatus(); });
    this->liveStatusTimer_.start(60 * 1000);

    // --
    this->messageSuffix_.append(' ');
    this->messageSuffix_.append(QChar(0x206D));

    // debugging
#if 0
    for (int i = 0; i < 1000; i++) {
        this->addMessage(Message::createSystemMessage("asdf"));
    }
#endif
}

bool TwitchChannel::isEmpty() const
{
    return this->name.isEmpty();
}

bool TwitchChannel::canSendMessage() const
{
    return !this->isEmpty();
}

void TwitchChannel::refreshChannelEmotes()
{
    auto app = getApp();

    Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    app->emotes->bttv.loadChannelEmotes(this->name, this->bttvEmotes_);
    app->emotes->ffz.loadChannelEmotes(this->name, this->ffzEmotes_);
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto app = getApp();

    if (!app->accounts->twitch.isLoggedIn()) {
        // XXX: It would be nice if we could add a link here somehow that opened the "account
        // manager" dialog
        this->addMessage(
            Message::createSystemMessage("You need to log in to send messages. You can "
                                         "link your Twitch account in the settings."));
        return;
    }

    Log("[TwitchChannel:{}] Send message: {}", this->name, message);

    // Do last message processing
    QString parsedMessage = app->emotes->emojis.replaceShortCodes(message);

    parsedMessage = parsedMessage.trimmed();

    if (parsedMessage.isEmpty()) {
        return;
    }

    if (!this->hasModRights()) {
        if (getSettings()->allowDuplicateMessages) {
            if (parsedMessage == this->lastSentMessage_) {
                parsedMessage.append(this->messageSuffix_);
            }
        }
    }

    bool messageSent = false;
    this->sendMessageSignal.invoke(this->name, parsedMessage, messageSent);

    if (messageSent) {
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
    if (this->mod_ != value) {
        this->mod_ = value;

        this->userStateChanged.invoke();
    }
}

bool TwitchChannel::isBroadcaster() const
{
    auto app = getApp();

    return this->name == app->accounts->twitch.getCurrent()->getUserName();
}

void TwitchChannel::addRecentChatter(const std::shared_ptr<Message> &message)
{
    assert(!message->loginName.isEmpty());

    this->completionModel.addUser(message->displayName);
}

void TwitchChannel::addJoinedUser(const QString &user)
{
    auto app = getApp();
    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !getSettings()->showJoins.getValue()) {
        return;
    }

    auto joinedUsers = this->joinedUsers_.access();
    joinedUsers->append(user);

    if (!this->joinedUsersMergeQueued_) {
        this->joinedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto joinedUsers = this->joinedUsers_.access();

            auto message = Message::createSystemMessage("Users joined: " + joinedUsers->join(", "));
            message->flags |= Message::Collapsed;
            joinedUsers->clear();
            this->addMessage(message);
            this->joinedUsersMergeQueued_ = false;
        });
    }
}

void TwitchChannel::addPartedUser(const QString &user)
{
    auto app = getApp();

    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !getSettings()->showJoins.getValue()) {
        return;
    }

    auto partedUsers = this->partedUsers_.access();
    partedUsers->append(user);

    if (!this->partedUsersMergeQueued_) {
        this->partedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto partedUsers = this->partedUsers_.access();

            auto message = Message::createSystemMessage("Users parted: " + partedUsers->join(", "));
            message->flags |= Message::Collapsed;
            this->addMessage(message);
            partedUsers->clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
}

QString TwitchChannel::getRoomId() const
{
    return this->roomID_.get();
}

void TwitchChannel::setRoomId(const QString &id)
{
    this->roomID_.set(id);
    this->roomIdChanged.invoke();
    this->loadRecentMessages();
}

const AccessGuard<TwitchChannel::RoomModes> TwitchChannel::accessRoomModes() const
{
    return this->roomModes_.access();
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

const AccessGuard<TwitchChannel::StreamStatus> TwitchChannel::accessStreamStatus() const
{
    return this->streamStatus_.access();
}

const EmoteMap &TwitchChannel::getFfzEmotes() const
{
    return *this->ffzEmotes_;
}

const EmoteMap &TwitchChannel::getBttvEmotes() const
{
    return *this->bttvEmotes_;
}

const QString &TwitchChannel::getSubscriptionUrl()
{
    return this->subscriptionUrl_;
}

const QString &TwitchChannel::getChannelUrl()
{
    return this->channelUrl_;
}

const QString &TwitchChannel::getPopoutPlayerUrl()
{
    return this->popoutPlayerUrl_;
}

void TwitchChannel::setLive(bool newLiveStatus)
{
    bool gotNewLiveStatus = false;
    {
        auto guard = this->streamStatus_.access();
        if (guard->live != newLiveStatus) {
            gotNewLiveStatus = true;
            guard->live = newLiveStatus;
        }
    }

    if (gotNewLiveStatus) {
        this->liveStatusChanged.invoke();
    }
}

void TwitchChannel::refreshLiveStatus()
{
    auto roomID = this->getRoomId();

    if (roomID.isEmpty()) {
        Log("[TwitchChannel:{}] Refreshing live status (Missing ID)", this->name);
        this->setLive(false);
        return;
    }

    Log("[TwitchChannel:{}] Refreshing live status", this->name);

    QString url("https://api.twitch.tv/kraken/streams/" + roomID);

    //<<<<<<< HEAD
    //    auto request = makeGetStreamRequest(roomID, QThread::currentThread());
    //=======
    //    auto request = NetworkRequest::twitchRequest(url);
    //    request.setCaller(QThread::currentThread());
    //>>>>>>> 9bfbdefd2f0972a738230d5b95a009f73b1dd933

    //    request.onSuccess([this, weak = this->weak_from_this()](auto result) {
    //        ChannelPtr shared = weak.lock();
    //        if (!shared) return false;

    //        return this->parseLiveStatus(result.parseRapidJson());
    //    });

    //    request.execute();
}

bool TwitchChannel::parseLiveStatus(const rapidjson::Document &document)
{
    if (!document.IsObject()) {
        Log("[TwitchChannel:refreshLiveStatus] root is not an object");
        return false;
    }

    if (!document.HasMember("stream")) {
        Log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
        return false;
    }

    const auto &stream = document["stream"];

    if (!stream.IsObject()) {
        // Stream is offline (stream is most likely null)
        this->setLive(false);
        return false;
    }

    if (!stream.HasMember("viewers") || !stream.HasMember("game") || !stream.HasMember("channel") ||
        !stream.HasMember("created_at")) {
        Log("[TwitchChannel:refreshLiveStatus] Missing members in stream");
        this->setLive(false);
        return false;
    }

    const rapidjson::Value &streamChannel = stream["channel"];

    if (!streamChannel.IsObject() || !streamChannel.HasMember("status")) {
        Log("[TwitchChannel:refreshLiveStatus] Missing member \"status\" in channel");
        return false;
    }

    // Stream is live

    {
        auto status = this->streamStatus_.access();
        status->live = true;
        status->viewerCount = stream["viewers"].GetUint();
        status->game = stream["game"].GetString();
        status->title = streamChannel["status"].GetString();
        QDateTime since = QDateTime::fromString(stream["created_at"].GetString(), Qt::ISODate);
        auto diff = since.secsTo(QDateTime::currentDateTime());
        status->uptime =
            QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

        status->rerun = false;
        if (stream.HasMember("stream_type")) {
            status->streamType = stream["stream_type"].GetString();
        } else {
            status->streamType = QString();
        }

        if (stream.HasMember("broadcast_platform")) {
            const auto &broadcastPlatformValue = stream["broadcast_platform"];

            if (broadcastPlatformValue.IsString()) {
                const char *broadcastPlatform = stream["broadcast_platform"].GetString();
                if (strcmp(broadcastPlatform, "rerun") == 0) {
                    status->rerun = true;
                }
            }
        }
    }

    // Signal all listeners that the stream status has been updated
    this->liveStatusChanged.invoke();

    return true;
}

void TwitchChannel::loadRecentMessages()
{
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" + getDefaultClientID();

    NetworkRequest request(genericURL.arg(this->getRoomId()));
    request.makeAuthorizedV5(getDefaultClientID());
    request.setCaller(QThread::currentThread());

    request.onSuccess([this, weak = this->weak_from_this()](auto result) {
        // channel still exists?
        ChannelPtr shared = weak.lock();
        if (!shared) return false;

        // parse json
        return this->parseRecentMessages(result.parseJson());
    });

    request.execute();
}

bool TwitchChannel::parseRecentMessages(const QJsonObject &jsonRoot)
{
    QJsonArray jsonMessages = jsonRoot.value("messages").toArray();
    if (jsonMessages.empty()) {
        return false;
    }

    std::vector<MessagePtr> messages;

    for (const auto jsonMessage : jsonMessages) {
        auto content = jsonMessage.toString().toUtf8();
        // passing nullptr as the channel makes the message invalid but we don't check for that
        // anyways
        auto message = Communi::IrcMessage::fromData(content, nullptr);
        auto privMsg = dynamic_cast<Communi::IrcPrivateMessage *>(message);
        assert(privMsg);

        MessageParseArgs args;
        TwitchMessageBuilder builder(this, privMsg, args);
        if (!builder.isIgnored()) {
            messages.push_back(builder.build());
        }
    }

    this->addMessagesAtStart(messages);

    return true;
}

void TwitchChannel::refreshPubsub()
{
    // listen to moderation actions
    if (!this->hasModRights()) return;
    auto roomId = this->getRoomId();
    if (roomId.isEmpty()) return;

    auto account = getApp()->accounts->twitch.getCurrent();
    getApp()->twitch2->pubsub->listenToChannelModerationActions(roomId, account);
}

void TwitchChannel::refreshViewerList()
{
    // setting?
    const auto streamStatus = this->accessStreamStatus();

    if (getSettings()->onlyFetchChattersForSmallerStreamers) {
        if (streamStatus->live && streamStatus->viewerCount > getSettings()->smallStreamerLimit) {
            return;
        }
    }

    // get viewer list
    NetworkRequest request("https://tmi.twitch.tv/group/user/" + this->name + "/chatters");

    request.setCaller(QThread::currentThread());
    request.onSuccess([this, weak = this->weak_from_this()](auto result) {
        // channel still exists?
        auto shared = weak.lock();
        if (!shared) return false;

        return this->parseViewerList(result.parseJson());
    });

    request.execute();
}

bool TwitchChannel::parseViewerList(const QJsonObject &jsonRoot)
{
    static QStringList categories = {"moderators", "staff", "admins", "global_mods", "viewers"};

    // parse json
    QJsonObject jsonCategories = jsonRoot.value("chatters").toObject();

    for (const auto &category : categories) {
        for (const auto jsonCategory : jsonCategories.value(category).toArray()) {
            this->completionModel.addUser(jsonCategory.toString());
        }
    }

    return true;
}

}  // namespace chatterino
