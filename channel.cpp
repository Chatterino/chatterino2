#include "channel.h"
#include "messages/message.h"
#include "windows.h"

#include <memory>

namespace chatterino {

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
    //    for (int i = 0; i < 40; i++) {
    //        addMessage(std::shared_ptr<messages::Message>(
    //            new messages::Message("test xD test")));
    //    }
}

QVector<std::shared_ptr<messages::Message>>
Channel::getMessagesClone()
{
    this->messageMutex.lock();
    QVector<std::shared_ptr<messages::Message>> M(this->messages);
    M.detach();
    this->messageMutex.unlock();
    return M;
}

void
Channel::addMessage(std::shared_ptr<messages::Message> message)
{
    this->messageMutex.lock();
    this->messages.append(message);
    this->messageMutex.unlock();

    Windows::repaintVisibleChatWidgets();
}
}
