#include "channel.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "logging/loggingmanager.hpp"
#include "messages/message.hpp"
#include "windowmanager.hpp"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

using namespace chatterino::messages;

namespace chatterino {

Channel::Channel(WindowManager &_windowManager, EmoteManager &_emoteManager,
                 IrcManager &_ircManager, const QString &channel, bool isSpecial)
    : windowManager(_windowManager)
    , emoteManager(_emoteManager)
    , ircManager(_ircManager)
    , _name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1) : channel)
    , bttvChannelEmotes(this->emoteManager.bttvChannels[channel])
    , ffzChannelEmotes(this->emoteManager.ffzChannels[channel])
    , _subLink("https://www.twitch.tv/" + _name + "/subscribe?ref=in_chat_subscriber_link")
    , _channelLink("https://twitch.tv/" + _name)
    , _popoutPlayerLink("https://player.twitch.tv/?channel=" + _name)
//    , _loggingChannel(logging::get(_name))
{
    qDebug() << "Open channel:" << channel << ". Name: " << _name;
    printf("Channel pointer: %p\n", this);
    if (!isSpecial) {
        this->reloadChannelEmotes();
    }
}

//
// properties
//
EmoteManager::EmoteMap &Channel::getBTTVChannelEmotes()
{
    return this->bttvChannelEmotes;
}

EmoteManager::EmoteMap &Channel::getFFZChannelEmotes()
{
    return this->ffzChannelEmotes;
}

bool Channel::isEmpty() const
{
    return _name.isEmpty();
}

const QString &Channel::getName() const
{
    return _name;
}

const QString &Channel::getSubLink() const
{
    return _subLink;
}

const QString &Channel::getChannelLink() const
{
    return _channelLink;
}

const QString &Channel::getPopoutPlayerLink() const
{
    return _popoutPlayerLink;
}

bool Channel::getIsLive() const
{
    return _isLive;
}

int Channel::getStreamViewerCount() const
{
    return _streamViewerCount;
}

const QString &Channel::getStreamStatus() const
{
    return _streamStatus;
}

const QString &Channel::getStreamGame() const
{
    return _streamGame;
}

messages::LimitedQueueSnapshot<messages::SharedMessage> Channel::getMessageSnapshot()
{
    return _messages.getSnapshot();
}

//
// methods
//
void Channel::addMessage(std::shared_ptr<Message> message)
{
    std::shared_ptr<Message> deleted;

    //    if (_loggingChannel.get() != nullptr) {
    //        _loggingChannel->append(message);
    //    }

    if (_messages.appendItem(message, deleted)) {
        messageRemovedFromStart(deleted);
    }

    this->messageAppended(message);

    this->windowManager.repaintVisibleChatWidgets(this);
}

// private methods
void Channel::reloadChannelEmotes()
{
    printf("[Channel:%s] Reloading channel emotes\n", qPrintable(this->_name));
    this->emoteManager.reloadBTTVChannelEmotes(this->_name);
    this->emoteManager.reloadFFZChannelEmotes(this->_name);
}

void Channel::sendMessage(const QString &message)
{
    qDebug() << "Channel send message: " << message;
    this->ircManager.sendMessage(_name, message);
}

}  // namespace chatterino
