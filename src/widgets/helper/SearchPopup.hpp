#pragma once

#if 0
#    include "ab/BaseWindow.hpp"

#    include <memory>

class QLineEdit;

namespace chatterino
{
    class Channel;
    class ChannelView;

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class SearchPopup : public ab::BaseWindow
    {
    public:
        SearchPopup();

        void setChannel(std::shared_ptr<Channel> channel);

    private:
        void initLayout();
        void performSearch();

        // LimitedQueueSnapshot<MessagePtr> snapshot_;
        QLineEdit* searchInput_;
        ChannelView* channelView_;
    };
}  // namespace chatterino
#endif
