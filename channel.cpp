#include "channel.h"
#include "message.h"
#include "windows.h"

#include <memory>

Channel::Channel(const QString &channel)
    : m_messages()
    , m_name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1)
                                                         : channel)
    , m_bttvChannelEmotes()
    , m_ffzChannelEmotes()
    , m_messageMutex()
    , m_subLink("https://www.twitch.tv/" + m_name +
                "/subscribe?ref=in_chat_subscriber_link")
    , m_channelLink("https://twitch.tv/" + m_name)
    , m_popoutPlayerLink("https://player.twitch.tv/?channel=" + m_name)
{
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

    Windows::repaintVisibleChatWidgets();
}
