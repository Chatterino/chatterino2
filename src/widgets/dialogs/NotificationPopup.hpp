#pragma once

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/BaseWindow.hpp"

namespace chatterino {

class ChannelView;

class NotificationPopup : public BaseWindow
{
public:
    enum Location { TopLeft, TopRight, BottomLeft, BottomRight };
    NotificationPopup();

    void addMessage(MessagePtr msg);
    void updatePosition();

private:
    ChannelView *channelView_;
    ChannelPtr channel_;
};

}  // namespace chatterino
