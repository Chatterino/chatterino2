#include "channel.h"
#include "message.h"

const Channel Channel::whispers = Channel(QString("/whispers"));
const Channel Channel::mentions = Channel(QString("/mentions"));

QMap<QString, Channel*> Channel::channels = QMap<QString, Channel*>();

Channel::Channel(QString channel)
    : m_name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1) : channel)
{
    messageMutex = new QMutex();
    subLink = "https://www.twitch.tv/" + m_name + "/subscribe?ref=in_chat_subscriber_link";
    channelLink = "https://twitch.tv/" + m_name;
    popoutPlayerLink = "https://player.twitch.tv/?channel=" + m_name;
}

//Channel::~Channel()
//{
////    delete messages;
//}

Channel* Channel::addChannel(const QString &channel)
{
    auto c = getChannel(channel);

    if (c == NULL) {
        c = new Channel(channel);
        channels.insert(channel, c);

        return c;
    }

    c->referenceCount++;

    return c;
}

Channel* Channel::getChannel(const QString &channel)
{
    if (channel == "/whispers") {
        return const_cast<Channel*>(&whispers);
    }

    if (channel == "/mentions") {
        return const_cast<Channel*>(&mentions);
    }

    auto a = channels.find(channel);

    if (a == channels.end()) {
        return NULL;
    }

    return a.value();
}

void Channel::removeChannel(const QString &channel)
{
    auto c = getChannel(channel);

    if (c == NULL) return;

    c->referenceCount--;

    if (c->referenceCount == 0) {
        channels.remove(channel);
        delete c;
    }
}

QString Channel::getSubLink()            { return subLink          ; }
QString Channel::getChannelLink()        { return channelLink      ; }
QString Channel::getPopoutPlayerLink()   { return popoutPlayerLink ; }

bool    Channel::getIsLive()             { return isLive           ; }
int     Channel::getStreamViewerCount()  { return streamViewerCount; }
QString Channel::getStreamStatus()       { return streamStatus     ; }
QString Channel::getStreamGame()         { return streamGame       ; }

QVector<Message*>* Channel::getMessagesClone()
{
    messageMutex->lock();
    auto M = new QVector<Message*>(*messages);
    messageMutex->unlock();
    return M;
}

void Channel::addMessage(Message *message)
{
    messageMutex->lock();
//    messages
    messageMutex->unlock();
}
