#pragma once

#include "channel.hpp"
#include "messages/message.hpp"
#include "widgets/basewindow.hpp"

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
