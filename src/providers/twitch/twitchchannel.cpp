#include "providers/twitch/twitchchannel.hpp"

#include "common.hpp"
#include "debug/log.hpp"
#include "messages/message.hpp"
#include "providers/twitch/pubsub.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/posttothread.hpp"
#include "util/urlfetch.hpp"

#include <IrcConnection>
#include <QThread>
#include <QTimer>

namespace chatterino {
namespace providers {
namespace twitch {

TwitchChannel::TwitchChannel(const QString &channelName, Communi::IrcConnection *_readConnection)
    : Channel(channelName, Channel::Twitch)
    , bttvChannelEmotes(new util::EmoteMap)
    , ffzChannelEmotes(new util::EmoteMap)
    , subscriptionURL("https://www.twitch.tv/subs/" + name)
    , channelURL("https://twitch.tv/" + name)
    , popoutPlayerURL("https://player.twitch.tv/?channel=" + name)
    , mod(false)
    , readConnection(_readConnection)
{
    debug::Log("[TwitchChannel:{}] Opened", this->name);

    auto app = getApp();
    this->reloadChannelEmotes();

    this->liveStatusTimer = new QTimer;
    QObject::connect(this->liveStatusTimer, &QTimer::timeout, [this]() {
        this->refreshLiveStatus();  //
    });
    this->liveStatusTimer->start(60000);

    this->roomIDchanged.connect([this]() {
        this->refreshLiveStatus();  //
    });

    this->managedConnect(app->accounts->Twitch.currentUserChanged,
                         [this]() { this->setMod(false); });

    auto refreshPubSubState = [=]() {
        if (!this->hasModRights()) {
            return;
        }

        if (this->roomID.isEmpty()) {
            return;
        }

        auto account = app->accounts->Twitch.getCurrent();
        if (account && !account->getUserId().isEmpty()) {
            app->twitch.pubsub->listenToChannelModerationActions(this->roomID, account);
        }
    };

    this->userStateChanged.connect(refreshPubSubState);
    this->roomIDchanged.connect(refreshPubSubState);
    this->managedConnect(app->accounts->Twitch.currentUserChanged, refreshPubSubState);
    refreshPubSubState();

    this->fetchMessages.connect([this] {
        this->fetchRecentMessages();  //
    });

    this->messageSuffix.append(' ');
    this->messageSuffix.append(QChar(0x206D));

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
        const auto streamStatus = this->GetStreamStatus();

        if (app->settings->onlyFetchChattersForSmallerStreamers) {
            if (streamStatus.live && streamStatus.viewerCount > app->settings->smallStreamerLimit) {
                return;
            }
        }

        util::twitch::get("https://tmi.twitch.tv/group/user/" + this->name + "/chatters",
                          QThread::currentThread(), refreshChatters);
    };

    doRefreshChatters();

    this->chattersListTimer = new QTimer;
    QObject::connect(this->chattersListTimer, &QTimer::timeout, doRefreshChatters);
    this->chattersListTimer->start(5 * 60 * 1000);

    for (int i = 0; i < 1000; i++) {
        this->addMessage(messages::Message::createSystemMessage("asdf"));
    }
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

    debug::Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    app->emotes->reloadBTTVChannelEmotes(this->name, this->bttvChannelEmotes);
    app->emotes->reloadFFZChannelEmotes(this->name, this->ffzChannelEmotes);
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto app = getApp();

    debug::Log("[TwitchChannel:{}] Send message: {}", this->name, message);

    // Do last message processing
    QString parsedMessage = app->emotes->replaceShortCodes(message);

    parsedMessage = parsedMessage.trimmed();

    if (parsedMessage.isEmpty()) {
        return;
    }

    if (app->settings->allowDuplicateMessages) {
        if (parsedMessage == this->lastSentMessage) {
            parsedMessage.append(this->messageSuffix);

            this->lastSentMessage = "";
        }
    }

    this->lastSentMessage = parsedMessage;

    this->sendMessageSignal.invoke(this->name, parsedMessage);
}

bool TwitchChannel::isMod() const
{
    return this->mod;
}

void TwitchChannel::setMod(bool value)
{
    if (this->mod != value) {
        this->mod = value;

        this->userStateChanged.invoke();
    }
}

bool TwitchChannel::isBroadcaster()
{
    auto app = getApp();

    return this->name == app->accounts->Twitch.getCurrent()->getUserName();
}

bool TwitchChannel::hasModRights()
{
    // fourtf: check if staff
    return this->isMod() || this->isBroadcaster();
}

void TwitchChannel::addRecentChatter(const std::shared_ptr<messages::Message> &message)
{
    assert(!message->loginName.isEmpty());

    std::lock_guard<std::mutex> lock(this->recentChattersMutex);

    this->recentChatters[message->loginName] = {message->displayName, message->localizedName};

    this->completionModel.addUser(message->displayName);
}

void TwitchChannel::setLive(bool newLiveStatus)
{
    bool gotNewLiveStatus = false;
    {
        std::lock_guard<std::mutex> lock(this->streamStatusMutex);
        if (this->streamStatus.live != newLiveStatus) {
            gotNewLiveStatus = true;
            this->streamStatus.live = newLiveStatus;
        }
    }

    if (gotNewLiveStatus) {
        this->updateLiveInfo.invoke();
    }
}

void TwitchChannel::refreshLiveStatus()
{
    if (this->roomID.isEmpty()) {
        this->setLive(false);
        return;
    }

    debug::Log("[TwitchChannel:{}] Refreshing live status", this->name);

    QString url("https://api.twitch.tv/kraken/streams/" + this->roomID);

    std::weak_ptr<Channel> weak = this->shared_from_this();

    util::twitch::get2(url, QThread::currentThread(), false, [weak](const rapidjson::Document &d) {
        ChannelPtr shared = weak.lock();

        if (!shared) {
            return;
        }

        TwitchChannel *channel = dynamic_cast<TwitchChannel *>(shared.get());

        if (!d.IsObject()) {
            debug::Log("[TwitchChannel:refreshLiveStatus] root is not an object");
            return;
        }

        if (!d.HasMember("stream")) {
            debug::Log("[TwitchChannel:refreshLiveStatus] Missing stream in root");
            return;
        }

        const auto &stream = d["stream"];

        if (!stream.IsObject()) {
            // Stream is offline (stream is most likely null)
            channel->setLive(false);
            return;
        }

        if (!stream.HasMember("viewers") || !stream.HasMember("game") ||
            !stream.HasMember("channel") || !stream.HasMember("created_at")) {
            debug::Log("[TwitchChannel:refreshLiveStatus] Missing members in stream");
            channel->setLive(false);
            return;
        }

        const rapidjson::Value &streamChannel = stream["channel"];

        if (!streamChannel.IsObject() || !streamChannel.HasMember("status")) {
            debug::Log("[TwitchChannel:refreshLiveStatus] Missing member \"status\" in channel");
            return;
        }

        // Stream is live

        {
            std::lock_guard<std::mutex> lock(channel->streamStatusMutex);
            channel->streamStatus.viewerCount = stream["viewers"].GetUint();
            channel->streamStatus.game = stream["game"].GetString();
            channel->streamStatus.title = streamChannel["status"].GetString();
            QDateTime since = QDateTime::fromString(stream["created_at"].GetString(), Qt::ISODate);
            auto diff = since.secsTo(QDateTime::currentDateTime());
            channel->streamStatus.uptime =
                QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

            channel->streamStatus.rerun = false;

            if (stream.HasMember("broadcast_platform")) {
                const auto &broadcastPlatformValue = stream["broadcast_platform"];

                if (broadcastPlatformValue.IsString()) {
                    const char *broadcastPlatform = stream["broadcast_platform"].GetString();
                    if (strcmp(broadcastPlatform, "rerun") == 0) {
                        channel->streamStatus.rerun = true;
                    }
                }
            }
        }

        channel->setLive(true);
    });
}

void TwitchChannel::fetchRecentMessages()
{
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" + getDefaultClientID();

    std::weak_ptr<Channel> weak = this->shared_from_this();

    util::twitch::get(genericURL.arg(roomID), QThread::currentThread(), [weak](QJsonObject obj) {
        ChannelPtr shared = weak.lock();

        if (!shared) {
            return;
        }

        auto channel = dynamic_cast<TwitchChannel *>(shared.get());
        assert(channel != nullptr);

        static auto readConnection = channel->readConnection;

        QJsonArray msgArray = obj.value("messages").toArray();
        if (msgArray.empty()) {
            return;
        }

        std::vector<messages::MessagePtr> messages;

        for (const QJsonValueRef _msg : msgArray) {
            QByteArray content = _msg.toString().toUtf8();
            auto msg = Communi::IrcMessage::fromData(content, readConnection);
            auto privMsg = static_cast<Communi::IrcPrivateMessage *>(msg);

            messages::MessageParseArgs args;
            twitch::TwitchMessageBuilder builder(channel, privMsg, args);
            if (!builder.isIgnored()) {
                messages.push_back(builder.build());
            }
        }
        channel->addMessagesAtStart(messages);
    });
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
