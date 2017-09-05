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

using namespace chatterino::messages;

namespace chatterino {

Channel::Channel(WindowManager &_windowManager, EmoteManager &_emoteManager,
                 IrcManager &_ircManager, const QString &channelName, bool isSpecial)
    : windowManager(_windowManager)
    , emoteManager(_emoteManager)
    , ircManager(_ircManager)
    , name(channelName)
    , bttvChannelEmotes(this->emoteManager.bttvChannels[channelName])
    , ffzChannelEmotes(this->emoteManager.ffzChannels[channelName])
    , _subLink("https://www.twitch.tv/" + name + "/subscribe?ref=in_chat_subscriber_link")
    , _channelLink("https://twitch.tv/" + name)
    , _popoutPlayerLink("https://player.twitch.tv/?channel=" + name)
    , isLive(false)
//    , _loggingChannel(logging::get(_name))
{
    qDebug() << "Open channel:" << this->name;

    if (!isSpecial) {
        this->reloadChannelEmotes();
    }
}

//
// properties
//

bool Channel::isEmpty() const
{
    return name.isEmpty();
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

void Channel::setRoomID(std::string id)
{
    this->roomID = id;
    this->roomIDchanged();
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
    printf("[Channel:%s] Reloading channel emotes\n", qPrintable(this->name));
    this->emoteManager.reloadBTTVChannelEmotes(this->name);
    this->emoteManager.reloadFFZChannelEmotes(this->name);
}

void Channel::sendMessage(const QString &message)
{
    qDebug() << "Channel send message: " << message;

    // Do last message processing
    QString parsedMessage = this->emoteManager.replaceShortCodes(message);

    this->ircManager.sendMessage(name, parsedMessage);
}

}  // namespace chatterino
