#include "channel.h"
#include "message.h"
#include "windows.h"

#include <memory>

Channel::Channel(const QString &channel)
    : messages()
    , name((channel.length() > 0 && channel[0] == '#') ? channel.mid(1)
                                                       : channel)
    , bttvChannelEmotes()
    , ffzChannelEmotes()
    , messageMutex()
    , subLink("https://www.twitch.tv/" + name +
              "/subscribe?ref=in_chat_subscriber_link")
    , channelLink("https://twitch.tv/" + name)
    , popoutPlayerLink("https://player.twitch.tv/?channel=" + name)
{
}

QVector<std::shared_ptr<Message>>
Channel::getMessagesClone()
{
    this->messageMutex.lock();
    QVector<std::shared_ptr<Message>> M(this->messages);
    M.detach();
    this->messageMutex.unlock();
    return M;
}

void
Channel::addMessage(std::shared_ptr<Message> message)
{
    this->messageMutex.lock();
    this->messages.append(message);
    this->messageMutex.unlock();

    Windows::repaintVisibleChatWidgets();
}
