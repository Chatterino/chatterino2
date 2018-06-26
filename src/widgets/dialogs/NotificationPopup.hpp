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

    void addMessage(chatterino::MessagePtr msg);
    void updatePosition();

private:
    ChannelView *channelView;
    ChannelPtr channel;
};

}  // namespace chatterino
