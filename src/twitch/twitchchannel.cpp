#include "twitchchannel.hpp"
#include "debug/log.hpp"
#include "emotemanager.hpp"
#include "util/urlfetch.hpp"

#include <QThread>
#include <QTimer>

namespace chatterino {
namespace twitch {

TwitchChannel::TwitchChannel(EmoteManager &emoteManager, IrcManager &ircManager,
                             const QString &channelName, bool _isSpecial)
    : Channel(channelName)
    , emoteManager(emoteManager)
    , ircManager(ircManager)
    , bttvChannelEmotes(new EmoteMap)
    , ffzChannelEmotes(new EmoteMap)
    , subscriptionURL("https://www.twitch.tv/" + name + "/subscribe?ref=in_chat_subscriber_link")
    , channelURL("https://twitch.tv/" + name)
    , popoutPlayerURL("https://player.twitch.tv/?channel=" + name)
    , isLive(false)
    , isSpecial(_isSpecial)
{
    debug::Log("[TwitchChannel:{}] Opened", this->name);

    if (!this->isSpecial) {
        this->reloadChannelEmotes();
    }

    this->liveStatusTimer = new QTimer;
    QObject::connect(this->liveStatusTimer, &QTimer::timeout, [this]() {
        this->refreshLiveStatus();  //
    });
    this->liveStatusTimer->start(60000);

    this->roomIDchanged.connect([this]() {
        this->refreshLiveStatus();  //
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
    return !this->isEmpty() && !this->isSpecial;
}

void TwitchChannel::setRoomID(const QString &_roomID)
{
    this->roomID = _roomID;
    this->roomIDchanged();
}

void TwitchChannel::reloadChannelEmotes()
{
    debug::Log("[TwitchChannel:{}] Reloading channel emotes", this->name);

    this->emoteManager.reloadBTTVChannelEmotes(this->name, this->bttvChannelEmotes);
    this->emoteManager.reloadFFZChannelEmotes(this->name, this->ffzChannelEmotes);
}

void TwitchChannel::sendMessage(const QString &message)
{
    debug::Log("[TwitchChannel:{}] Send message: {}", this->name, message);

    // Do last message processing
    QString parsedMessage = this->emoteManager.replaceShortCodes(message);

    this->ircManager.sendMessage(this->name, parsedMessage);
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

    util::twitch::get(url, QThread::currentThread(), [this](QJsonObject obj) {
        if (obj.value("stream").isNull()) {
            this->setLive(false);
        } else {
            auto stream = obj.value("stream").toObject();
            this->streamViewerCount = QString::number(stream.value("viewers").toDouble());
            this->streamGame = stream.value("game").toString();
            this->streamStatus = stream.value("channel").toObject().value("status").toString();
            QDateTime since =
                QDateTime::fromString(stream.value("created_at").toString(), Qt::ISODate);
            auto diff = since.secsTo(QDateTime::currentDateTime());
            this->streamUptime =
                QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";

            this->setLive(true);
        }
    });
}

}  // namespace twitch
}  // namespace chatterino
