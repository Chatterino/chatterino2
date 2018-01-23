#include "twitchchannel.hpp"
#include "debug/log.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "util/urlfetch.hpp"

#include <QThread>
#include <QTimer>

namespace chatterino {
namespace twitch {

TwitchChannel::TwitchChannel(const QString &channelName)
    : Channel(channelName)
    , bttvChannelEmotes(new util::EmoteMap)
    , ffzChannelEmotes(new util::EmoteMap)
    , subscriptionURL("https://www.twitch.tv/subs/" + name)
    , channelURL("https://twitch.tv/" + name)
    , popoutPlayerURL("https://player.twitch.tv/?channel=" + name)
    , isLive(false)
    , mod(false)
{
    debug::Log("[TwitchChannel:{}] Opened", this->name);

    this->reloadChannelEmotes();

    this->liveStatusTimer = new QTimer;
    QObject::connect(this->liveStatusTimer, &QTimer::timeout, [this]() {
        this->refreshLiveStatus();  //
    });
    this->liveStatusTimer->start(60000);

    this->roomIDchanged.connect([this]() {
        this->refreshLiveStatus();  //
    });

    this->fetchMessages.connect([this] {
        this->fetchRecentMessages();  //
    });

    this->connectedConnection = singletons::IrcManager::getInstance().connected.connect(
        [this] { this->userStateChanged(); });

    this->messageSuffix.append(' ');
    this->messageSuffix.append(QChar(0x206D));
}

TwitchChannel::~TwitchChannel()
{
    this->connectedConnection.disconnect();

    this->liveStatusTimer->stop();
    this->liveStatusTimer->deleteLater();
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
    this->roomIDchanged();
    this->fetchMessages.invoke();
}

void TwitchChannel::reloadChannelEmotes()
{
    auto &emoteManager = singletons::EmoteManager::getInstance();

    debug::Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    emoteManager.reloadBTTVChannelEmotes(this->name, this->bttvChannelEmotes);
    emoteManager.reloadFFZChannelEmotes(this->name, this->ffzChannelEmotes);
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto &emoteManager = singletons::EmoteManager::getInstance();

    debug::Log("[TwitchChannel:{}] Send message: {}", this->name, message);

    // Do last message processing
    QString parsedMessage = emoteManager.replaceShortCodes(message);

    parsedMessage = parsedMessage.trimmed();

    if (parsedMessage.isEmpty())
        return;

    if (singletons::SettingManager::getInstance().allowDuplicateMessages) {
        if (parsedMessage == this->lastSentMessage) {
            parsedMessage.append(this->messageSuffix);

            this->lastSentMessage = "";
        }
    }

    singletons::IrcManager::getInstance().sendMessage(this->name, parsedMessage);
}

bool TwitchChannel::isMod() const
{
    return this->mod;
}

void TwitchChannel::setMod(bool value)
{
    if (this->mod != value) {
        this->mod = value;

        this->userStateChanged();
    }
}

bool TwitchChannel::isBroadcaster()
{
    return this->name ==
           singletons::AccountManager::getInstance().Twitch.getCurrent()->getUserName();
}

bool TwitchChannel::hasModRights()
{
    // fourtf: check if staff
    return this->isMod() || this->isBroadcaster();
}

void TwitchChannel::setLive(bool newLiveStatus)
{
    if (this->isLive == newLiveStatus) {
        return;
    }

    this->isLive = newLiveStatus;
    this->onlineStatusChanged();
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

    util::twitch::get2(url, QThread::currentThread(), [weak](const rapidjson::Document &d) {
        SharedChannel shared = weak.lock();

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
        channel->streamViewerCount = QString::number(stream["viewers"].GetInt());
        channel->streamGame = stream["game"].GetString();
        channel->streamStatus = streamChannel["status"].GetString();
        QDateTime since = QDateTime::fromString(stream["created_at"].GetString(), Qt::ISODate);
        auto diff = since.secsTo(QDateTime::currentDateTime());
        channel->streamUptime =
            QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

        channel->setLive(true);
    });
}

void TwitchChannel::fetchRecentMessages()
{
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" + getDefaultClientID();

    std::weak_ptr<Channel> weak = this->shared_from_this();

    util::twitch::get(genericURL.arg(roomID), QThread::currentThread(), [weak](QJsonObject obj) {
        SharedChannel shared = weak.lock();

        if (!shared) {
            return;
        }

        TwitchChannel *channel = dynamic_cast<TwitchChannel *>(shared.get());
        static auto readConnection = singletons::IrcManager::getInstance().getReadConnection();

        auto msgArray = obj.value("messages").toArray();
        if (msgArray.size() > 0) {
            std::vector<messages::MessagePtr> messages;

            for (int i = 0; i < msgArray.size(); i++) {
                QByteArray content = msgArray[i].toString().toUtf8();
                auto msg = Communi::IrcMessage::fromData(content, readConnection);
                auto privMsg = static_cast<Communi::IrcPrivateMessage *>(msg);

                messages::MessageParseArgs args;
                twitch::TwitchMessageBuilder builder(channel, privMsg, args);
                if (!builder.isIgnored()) {
                    messages.push_back(builder.parse());
                }
            }
            channel->addMessagesAtStart(messages);
        }
    });
}
}  // namespace twitch
}  // namespace chatterino
