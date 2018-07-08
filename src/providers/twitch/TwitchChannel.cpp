#include "providers/twitch/TwitchChannel.hpp"

#include "common/Common.hpp"
#include "common/UrlFetch.hpp"
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

TwitchChannel::TwitchChannel(const QString &channelName, Communi::IrcConnection *readConnection)
    : Channel(channelName, Channel::Type::Twitch)
    , bttvChannelEmotes(new EmoteMap)
    , ffzChannelEmotes(new EmoteMap)
    , subscriptionURL("https://www.twitch.tv/subs/" + name)
    , channelURL("https://twitch.tv/" + name)
    , popoutPlayerURL("https://player.twitch.tv/?channel=" + name)
    , mod_(false)
    , readConnection_(readConnection)
{
    Log("[TwitchChannel:{}] Opened", this->name);

    this->startRefreshLiveStatusTimer(60 * 1000);

    auto app = getApp();
    this->reloadChannelEmotes();

    this->managedConnect(app->accounts->twitch.currentUserChanged,
                         [this]() { this->setMod(false); });

    auto refreshPubSubState = [=]() {
        if (!this->hasModRights()) {
            return;
        }

        if (this->roomID.isEmpty()) {
            return;
        }

        auto account = app->accounts->twitch.getCurrent();
        if (account && !account->getUserId().isEmpty()) {
            app->twitch.pubsub->listenToChannelModerationActions(this->roomID, account);
        }
    };

    this->userStateChanged.connect(refreshPubSubState);
    this->roomIDchanged.connect(refreshPubSubState);
    this->managedConnect(app->accounts->twitch.currentUserChanged, refreshPubSubState);
    refreshPubSubState();

    this->fetchMessages.connect([this] {
        this->fetchRecentMessages();  //
    });

    this->messageSuffix_.append(' ');
    this->messageSuffix_.append(QChar(0x206D));

    static QStringList jsonLabels = {"moderators", "staff", "admins", "global_mods", "viewers"};
    auto refreshChatters = [=](QJsonObject obj) {
        QJsonObject chattersObj = obj.value("chatters").toObject();
        for (int i = 0; i < jsonLabels.size(); i++) {
            foreach (const QJsonValue &v, chattersObj.value(jsonLabels.at(i)).toArray()) {
                this->completionModel.addUser(v.toString());
            }
        }
    };

    auto doRefreshChatters = [=]() {
        const auto streamStatus = this->getStreamStatus();

        if (app->settings->onlyFetchChattersForSmallerStreamers) {
            if (streamStatus.live && streamStatus.viewerCount > app->settings->smallStreamerLimit) {
                return;
            }
        }

        NetworkRequest request("https://tmi.twitch.tv/group/user/" + this->name + "/chatters");

        request.setCaller(QThread::currentThread());
        request.onSuccess([refreshChatters](auto result) {
            refreshChatters(result.parseJson());  //

            return true;
        });

        request.execute();
    };

    doRefreshChatters();

    this->chattersListTimer = new QTimer;
    QObject::connect(this->chattersListTimer, &QTimer::timeout, doRefreshChatters);
    this->chattersListTimer->start(5 * 60 * 1000);

#if 0
    for (int i = 0; i < 1000; i++) {
        this->addMessage(Message::createSystemMessage("asdf"));
    }
#endif
}

TwitchChannel::~TwitchChannel()
{
    this->liveStatusTimer->stop();
    this->liveStatusTimer->deleteLater();

    this->chattersListTimer->stop();
    this->chattersListTimer->deleteLater();
}

bool TwitchChannel::isEmpty() const
{
    return this->name.isEmpty();
}

bool TwitchChannel::canSendMessage() const
{
    return !this->isEmpty();
}

void TwitchChannel::setRoomID(const QString &_roomID)
{
    this->roomID = _roomID;
    this->roomIDchanged.invoke();
    this->fetchMessages.invoke();
}

void TwitchChannel::reloadChannelEmotes()
{
    auto app = getApp();

    Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    app->emotes->bttv.loadChannelEmotes(this->name, this->bttvChannelEmotes);
    app->emotes->ffz.loadChannelEmotes(this->name, this->ffzChannelEmotes);
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
        if (app->settings->allowDuplicateMessages) {
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

    std::lock_guard<std::mutex> lock(this->recentChattersMutex_);

    this->recentChatters_[message->loginName] = {message->displayName, message->localizedName};

    this->completionModel.addUser(message->displayName);
}

void TwitchChannel::addJoinedUser(const QString &user)
{
    auto *app = getApp();
    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !app->settings->showJoins.getValue()) {
        return;
    }

    std::lock_guard<std::mutex> guard(this->joinedUserMutex_);

    joinedUsers_ << user;

    if (!this->joinedUsersMergeQueued_) {
        this->joinedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->object_, [this] {
            std::lock_guard<std::mutex> guard(this->joinedUserMutex_);

            auto message =
                Message::createSystemMessage("Users joined: " + this->joinedUsers_.join(", "));
            message->flags |= Message::Collapsed;
            this->addMessage(message);
            this->joinedUsers_.clear();
            this->joinedUsersMergeQueued_ = false;
        });
    }
}

void TwitchChannel::addPartedUser(const QString &user)
{
    auto *app = getApp();

    if (user == app->accounts->twitch.getCurrent()->getUserName() ||
        !app->settings->showJoins.getValue()) {
        return;
    }

    std::lock_guard<std::mutex> guard(this->partedUserMutex_);

    partedUsers_ << user;

    if (!this->partedUsersMergeQueued_) {
        this->partedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->object_, [this] {
            std::lock_guard<std::mutex> guard(this->partedUserMutex_);

            auto message =
                Message::createSystemMessage("Users parted: " + this->partedUsers_.join(", "));
            message->flags |= Message::Collapsed;
            this->addMessage(message);
            this->partedUsers_.clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
}

TwitchChannel::RoomModes TwitchChannel::getRoomModes()
{
    std::lock_guard<std::mutex> lock(this->roomModeMutex_);

    return this->roomModes_;
}

void TwitchChannel::setRoomModes(const RoomModes &_roomModes)
{
    {
        std::lock_guard<std::mutex> lock(this->roomModeMutex_);
        this->roomModes_ = _roomModes;
    }

    this->roomModesChanged.invoke();
}

bool TwitchChannel::isLive() const
{
    std::lock_guard<std::mutex> lock(this->streamStatusMutex_);
    return this->streamStatus_.live;
}

TwitchChannel::StreamStatus TwitchChannel::getStreamStatus() const
{
    std::lock_guard<std::mutex> lock(this->streamStatusMutex_);
    return this->streamStatus_;
}

void TwitchChannel::setLive(bool newLiveStatus)
{
    bool gotNewLiveStatus = false;
    {
        std::lock_guard<std::mutex> lock(this->streamStatusMutex_);
        if (this->streamStatus_.live != newLiveStatus) {
            gotNewLiveStatus = true;
            this->streamStatus_.live = newLiveStatus;
        }
    }

    if (gotNewLiveStatus) {
        this->updateLiveInfo.invoke();
    }
}

void TwitchChannel::refreshLiveStatus()
{
    if (this->roomID.isEmpty()) {
        Log("[TwitchChannel:{}] Refreshing live status (Missing ID)", this->name);
        this->setLive(false);
        return;
    }

    Log("[TwitchChannel:{}] Refreshing live status", this->name);

    QString url("https://api.twitch.tv/kraken/streams/" + this->roomID);

    std::weak_ptr<Channel> weak = this->shared_from_this();

    auto request = makeGetStreamRequest(this->roomID, QThread::currentThread());

    request.onSuccess([weak](auto result) {
        auto d = result.parseRapidJson();
        ChannelPtr shared = weak.lock();

        if (!shared) {
            return false;
        }

        TwitchChannel *channel = dynamic_cast<TwitchChannel *>(shared.get());

        if (!d.IsObject()) {
            Log("[TwitchChannel:refreshLiveStatus] root is not an object");
            return false;
        }

        if (!d.HasMember("stream")) {
            Log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
            return false;
        }

        const auto &stream = d["stream"];

        if (!stream.IsObject()) {
            // Stream is offline (stream is most likely null)
            channel->setLive(false);
            return false;
        }

        if (!stream.HasMember("viewers") || !stream.HasMember("game") ||
            !stream.HasMember("channel") || !stream.HasMember("created_at")) {
            Log("[TwitchChannel:refreshLiveStatus] Missing members in stream");
            channel->setLive(false);
            return false;
        }

        const rapidjson::Value &streamChannel = stream["channel"];

        if (!streamChannel.IsObject() || !streamChannel.HasMember("status")) {
            Log("[TwitchChannel:refreshLiveStatus] Missing member \"status\" in channel");
            return false;
        }

        // Stream is live

        {
            std::lock_guard<std::mutex> lock(channel->streamStatusMutex_);
            channel->streamStatus_.live = true;
            channel->streamStatus_.viewerCount = stream["viewers"].GetUint();
            channel->streamStatus_.game = stream["game"].GetString();
            channel->streamStatus_.title = streamChannel["status"].GetString();
            QDateTime since = QDateTime::fromString(stream["created_at"].GetString(), Qt::ISODate);
            auto diff = since.secsTo(QDateTime::currentDateTime());
            channel->streamStatus_.uptime =
                QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

            channel->streamStatus_.rerun = false;
            if (stream.HasMember("stream_type")) {
                channel->streamStatus_.streamType = stream["stream_type"].GetString();
            } else {
                channel->streamStatus_.streamType = QString();
            }

            if (stream.HasMember("broadcast_platform")) {
                const auto &broadcastPlatformValue = stream["broadcast_platform"];

                if (broadcastPlatformValue.IsString()) {
                    const char *broadcastPlatform = stream["broadcast_platform"].GetString();
                    if (strcmp(broadcastPlatform, "rerun") == 0) {
                        channel->streamStatus_.rerun = true;
                    }
                }
            }
        }

        // Signal all listeners that the stream status has been updated
        channel->updateLiveInfo.invoke();

        return true;
    });

    request.execute();
}

void TwitchChannel::startRefreshLiveStatusTimer(int intervalMS)
{
    this->liveStatusTimer = new QTimer;
    QObject::connect(this->liveStatusTimer, &QTimer::timeout, [this]() {
        this->refreshLiveStatus();  //
    });

    // When the Room ID of a twitch channel has been set, refresh the live status an extra time
    this->roomIDchanged.connect([this]() {
        this->refreshLiveStatus();  //
    });

    this->liveStatusTimer->start(intervalMS);
}

void TwitchChannel::fetchRecentMessages()
{
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" + getDefaultClientID();

    NetworkRequest request(genericURL.arg(this->roomID));
    request.makeAuthorizedV5(getDefaultClientID());
    request.setCaller(QThread::currentThread());

    std::weak_ptr<Channel> weak = this->shared_from_this();

    request.onSuccess([weak](auto result) {
        auto obj = result.parseJson();
        ChannelPtr shared = weak.lock();

        if (!shared) {
            return false;
        }

        auto channel = dynamic_cast<TwitchChannel *>(shared.get());
        assert(channel != nullptr);

        static auto readConnection = channel->readConnection_;

        QJsonArray msgArray = obj.value("messages").toArray();
        if (msgArray.empty()) {
            return false;
        }

        std::vector<MessagePtr> messages;

        for (const QJsonValueRef _msg : msgArray) {
            QByteArray content = _msg.toString().toUtf8();
            auto msg = Communi::IrcMessage::fromData(content, readConnection);
            auto privMsg = static_cast<Communi::IrcPrivateMessage *>(msg);

            MessageParseArgs args;
            TwitchMessageBuilder builder(channel, privMsg, args);
            if (!builder.isIgnored()) {
                messages.push_back(builder.build());
            }
        }

        channel->addMessagesAtStart(messages);

        return true;
    });

    request.execute();
}

}  // namespace chatterino
