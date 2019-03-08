#pragma once

#if 0

#    include "ab/BaseWindow.hpp"

namespace chatterino
{
    class ChannelView;

    class Channel;
    using ChannelPtr = std::shared_ptr<Channel>;

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class NotificationPopup : public ab::BaseWindow
    {
    public:
        enum Location { TopLeft, TopRight, BottomLeft, BottomRight };
        NotificationPopup();

        void addMessage(MessagePtr msg);
        void updatePosition();

    private:
        ChannelView* channelView_;
        ChannelPtr channel_;
    };
}  // namespace chatterino
#endif
