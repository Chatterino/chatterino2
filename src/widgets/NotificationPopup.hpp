#pragma once

#include "Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/BaseWindow.hpp"

namespace chatterino {
namespace widgets {

class ChannelView;

class NotificationPopup : public BaseWindow
{
public:
    enum Location { TopLeft, TopRight, BottomLeft, BottomRight };
    NotificationPopup();

    void addMessage(messages::MessagePtr msg);
    void updatePosition();

private:
    ChannelView *channelView;
    ChannelPtr channel;
};

}  // namespace widgets
}  // namespace chatterino
