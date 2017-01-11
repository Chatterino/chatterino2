#include "channel.h"
#include "message.h"

#include <memory>

Channel Channel::whispers(QString("/whispers"));
Channel Channel::mentions(QString("/mentions"));

QMap<QString, Channel *> Channel::channels = QMap<QString, Channel *>();

Channel::Channel(QString channel)
    : m_name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1)
                                                         : channel)
    , m_bttvChannelEmotes()
    , m_ffzChannelEmotes()
    , m_messages()
    , m_messageMutex()
    , m_subLink("https://www.twitch.tv/" + m_name +
                "/subscribe?ref=in_chat_subscriber_link")
    , m_channelLink("https://twitch.tv/" + m_name)
    , m_popoutPlayerLink("https://player.twitch.tv/?channel=" + m_name)
{
}

Channel *
Channel::addChannel(const QString &channel)
{
    auto c = getChannel(channel);

    if (c == NULL) {
        c = new Channel(channel);
        channels.insert(channel, c);

        return c;
    }

    c->m_referenceCount++;

    return c;
}

Channel *
Channel::getChannel(const QString &channel)
{
    if (channel == "/whispers") {
        return const_cast<Channel *>(&whispers);
    }

    if (channel == "/mentions") {
        return const_cast<Channel *>(&mentions);
    }

    auto a = channels.find(channel);

    if (a == channels.end()) {
        return NULL;
    }

    return a.value();
}

void
Channel::removeChannel(const QString &channel)
{
    auto c = getChannel(channel);

    if (c == NULL)
        return;

    c->m_referenceCount--;

    if (c->m_referenceCount == 0) {
        channels.remove(channel);
        delete c;
    }
}

QVector<std::shared_ptr<Message>>
Channel::getMessagesClone()
{
    m_messageMutex.lock();
    QVector<std::shared_ptr<Message>> M(m_messages);
    M.detach();
    m_messageMutex.unlock();
    return M;
}

void
Channel::addMessage(std::shared_ptr<Message> message)
{
    m_messageMutex.lock();
    m_messages.append(message);
    m_messageMutex.unlock();
}
