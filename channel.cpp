#include "channel.h"
#include "emotemanager.h"
#include "ircmanager.h"
#include "logging/loggingmanager.h"
#include "messages/message.h"
#include "util/urlfetch.h"
#include "windowmanager.h"

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

Channel::Channel(const QString &channel)
    : _messages()
    , _name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1) : channel)
    , _bttvChannelEmotes()
    , _ffzChannelEmotes()
    , _subLink("https://www.twitch.tv/" + _name + "/subscribe?ref=in_chat_subscriber_link")
    , _channelLink("https://twitch.tv/" + _name)
    , _popoutPlayerLink("https://player.twitch.tv/?channel=" + _name)
//    , _loggingChannel(logging::get(_name))
{
    qDebug() << "Open channel:" << channel << ". Name: " << _name;
    printf("Channel pointer: %p\n", this);
    reloadChannelEmotes();
}

//
// properties
//
ConcurrentMap<QString, messages::LazyLoadedImage *> &Channel::getBttvChannelEmotes()
{
    return _bttvChannelEmotes;
}

ConcurrentMap<QString, messages::LazyLoadedImage *> &Channel::getFfzChannelEmotes()
{
    return _ffzChannelEmotes;
}

bool Channel::isEmpty() const
{
    return _name.isEmpty();
}

const QString &Channel::getName() const
{
    return _name;
}

int Channel::getRoomID() const
{
    return _roomID;
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

    WindowManager::getInstance().repaintVisibleChatWidgets(this);
}

// private methods
void Channel::reloadChannelEmotes()
{
    reloadBttvEmotes();
    reloadFfzEmotes();
}

void Channel::sendMessage(const QString &message)
{
    qDebug() << "Channel send message: " << message;
    IrcManager &instance = IrcManager::getInstance();
    instance.sendMessage(_name, message);
}

void Channel::reloadBttvEmotes()
{
    util::urlJsonFetch(
        "https://api.betterttv.net/2/channels/" + _name, [this](QJsonObject &rootNode) {
            auto emotesNode = rootNode.value("emotes").toArray();

            QString linkTemplate = "https:" + rootNode.value("urlTemplate").toString();

            for (const QJsonValue &emoteNode : emotesNode) {
                QJsonObject emoteObject = emoteNode.toObject();

                QString id = emoteObject.value("id").toString();
                QString code = emoteObject.value("code").toString();
                // emoteObject.value("imageType").toString();

                QString link = linkTemplate;
                link.detach();

                link = link.replace("{{id}}", id).replace("{{image}}", "1x");

                auto emote = EmoteManager::getInstance().getBttvChannelEmoteFromCaches().getOrAdd(
                    id, [&code, &link] {
                        return new LazyLoadedImage(link, 1, code, code + "\nChannel Bttv Emote");
                    });

                this->getBttvChannelEmotes().insert(code, emote);
            }
        });
}

void Channel::reloadFfzEmotes()
{
    util::urlJsonFetch("http://api.frankerfacez.com/v1/room/" + _name, [this](
                                                                           QJsonObject &rootNode) {
        auto setsNode = rootNode.value("sets").toObject();

        for (const QJsonValue &setNode : setsNode) {
            auto emotesNode = setNode.toObject().value("emoticons").toArray();

            for (const QJsonValue &emoteNode : emotesNode) {
                QJsonObject emoteObject = emoteNode.toObject();

                // margins

                int id = emoteObject.value("id").toInt();
                QString code = emoteObject.value("name").toString();

                QJsonObject urls = emoteObject.value("urls").toObject();
                QString url1 = "http:" + urls.value("1").toString();

                auto emote = EmoteManager::getInstance().getFfzChannelEmoteFromCaches().getOrAdd(
                    id, [&code, &url1] {
                        return new LazyLoadedImage(url1, 1, code, code + "\nGlobal Ffz Emote");
                    });

                getFfzChannelEmotes().insert(code, emote);
            }
        }
    });
}

}  // namespace chatterino
