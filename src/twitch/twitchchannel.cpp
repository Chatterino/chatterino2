#include "twitchchannel.hpp"
#include "debug/log.hpp"
#include "singletons/emotemanager.hpp"
#include "util/urlfetch.hpp"

#include <QThread>
#include <QTimer>

namespace chatterino {
namespace twitch {

TwitchChannel::TwitchChannel(const QString &channelName)
    : Channel(channelName)
    , bttvChannelEmotes(new EmoteMap)
    , ffzChannelEmotes(new EmoteMap)
    , subscriptionURL("https://www.twitch.tv/subs/" + name)
    , channelURL("https://twitch.tv/" + name)
    , popoutPlayerURL("https://player.twitch.tv/?channel=" + name)
    , isLive(false)
{
    debug::Log("[TwitchChannel:{}] Opened", this->name);

    this->dontAddMessages = true;

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
}

TwitchChannel::~TwitchChannel()
{
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
    auto &emoteManager = EmoteManager::getInstance();

    debug::Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    emoteManager.reloadBTTVChannelEmotes(this->name, this->bttvChannelEmotes);
    emoteManager.reloadFFZChannelEmotes(this->name, this->ffzChannelEmotes);
}

void TwitchChannel::sendMessage(const QString &message)
{
    auto &emoteManager = EmoteManager::getInstance();

    debug::Log("[TwitchChannel:{}] Send message: {}", this->name, message);

    // Do last message processing
    QString parsedMessage = emoteManager.replaceShortCodes(message);

    IrcManager::getInstance().sendMessage(this->name, parsedMessage);
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

    util::twitch::get2(url, QThread::currentThread(), [this](rapidjson::Document &d) {
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
            this->setLive(false);
            return;
        }

        if (!stream.HasMember("viewers") || !stream.HasMember("game") ||
            !stream.HasMember("channel") || !stream.HasMember("created_at")) {
            debug::Log("[TwitchChannel:refreshLiveStatus] Missing members in stream");
            this->setLive(false);
            return;
        }

        const rapidjson::Value &streamChannel = stream["channel"];

        if (!streamChannel.IsObject() || !streamChannel.HasMember("status")) {
            debug::Log("[TwitchChannel:refreshLiveStatus] Missing member \"status\" in channel");
            return;
        }

        // Stream is live
        this->streamViewerCount = QString::number(stream["viewers"].GetInt());
        this->streamGame = stream["game"].GetString();
        this->streamStatus = streamChannel["status"].GetString();
        QDateTime since = QDateTime::fromString(stream["created_at"].GetString(), Qt::ISODate);
        auto diff = since.secsTo(QDateTime::currentDateTime());
        this->streamUptime =
            QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

        this->setLive(true);
    });
}

void TwitchChannel::fetchRecentMessages()
{
    static QString genericURL =
        "https://tmi.twitch.tv/api/rooms/%1/recent_messages?client_id=" + getDefaultClientID();
    static auto readConnection = IrcManager::getInstance().getReadConnection();

    util::twitch::get(genericURL.arg(roomID), QThread::currentThread(), [=](QJsonObject obj) {
        this->dontAddMessages = false;
        auto msgArray = obj.value("messages").toArray();
        if (msgArray.size())
            for (int i = 0; i < msgArray.size(); i++) {
                QByteArray content = msgArray[i].toString().toUtf8();
                auto msg = Communi::IrcMessage::fromData(content, readConnection);
                auto privMsg = static_cast<Communi::IrcPrivateMessage *>(msg);
                IrcManager::getInstance().privateMessageReceived(privMsg);
            }
    });
}

}  // namespace twitch
}  // namespace chatterino
