#include "channel.h"
#include "message.h"

Channel Channel::whispers = Channel(QString("/whispers"));
Channel Channel::mentions = Channel(QString("/mentions"));

QMap<QString, Channel*> Channel::channels = QMap<QString, Channel*>();

Channel::Channel(QString channel)
    : m_name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1) : channel)
    , m_bttvChannelEmotes()
    , m_ffzChannelEmotes()
    , m_messages()
    , m_messageMutex()
    , m_subLink("https://www.twitch.tv/" + m_name + "/subscribe?ref=in_chat_subscriber_link")
    , m_channelLink("https://twitch.tv/" + m_name)
    , m_popoutPlayerLink("https://player.twitch.tv/?channel=" + m_name)
{

}

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

QVector<Message*> Channel::getMessagesClone()
{
    m_messageMutex.lock();
    QVector M = QVector<Message*>(*m_messages);
    m_messageMutex.unlock();
    return M;
}

void Channel::addMessage(Message *message)
{
    m_messageMutex.lock();
//    messages
    m_messageMutex.unlock();
}
