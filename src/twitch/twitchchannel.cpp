#include "twitchchannel.hpp"
#include "emotemanager.hpp"

#include <QDebug>

namespace chatterino {
namespace twitch {

TwitchChannel::TwitchChannel(EmoteManager &emoteManager, IrcManager &ircManager,
                             const QString &channelName, bool isSpecial)
    : emoteManager(emoteManager)
    , ircManager(ircManager)
    //    , name(channelName)
    , bttvChannelEmotes(new EmoteMap)
    , ffzChannelEmotes(new EmoteMap)
    , subLink("https://www.twitch.tv/" + name + "/subscribe?ref=in_chat_subscriber_link")
    , channelLink("https://twitch.tv/" + name)
    , popoutPlayerLink("https://player.twitch.tv/?channel=" + name)
    , isLive(false)
    , isSpecial(isSpecial)
{
    this->name = channelName;

    qDebug() << "Open twitch channel:" << this->name;

    if (!isSpecial) {
        this->reloadChannelEmotes();
    }
}

bool TwitchChannel::isEmpty() const
{
    return this->name.isEmpty();
}

const QString &TwitchChannel::getSubLink() const
{
    return this->subLink;
}

bool TwitchChannel::canSendMessage() const
{
    return !this->isEmpty() && !this->isSpecial;
}

const QString &TwitchChannel::getChannelLink() const
{
    return this->channelLink;
}

const QString &TwitchChannel::getPopoutPlayerLink() const
{
    return this->popoutPlayerLink;
}

void TwitchChannel::setRoomID(std::string id)
{
    this->roomID = id;
    this->roomIDchanged();
}

void TwitchChannel::reloadChannelEmotes()
{
    printf("[TwitchChannel:%s] Reloading channel emotes\n", qPrintable(this->name));

    this->emoteManager.reloadBTTVChannelEmotes(this->name, this->bttvChannelEmotes);
    this->emoteManager.reloadFFZChannelEmotes(this->name, this->ffzChannelEmotes);
}

void TwitchChannel::sendMessage(const QString &message)
{
    qDebug() << "TwitchChannel send message: " << message;

    // Do last message processing
    QString parsedMessage = this->emoteManager.replaceShortCodes(message);

    this->ircManager.sendMessage(this->name, parsedMessage);
}
}
}
