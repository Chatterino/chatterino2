#pragma once

#include "messages/LimitedQueueSnapshot.hpp"
#include "widgets/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino
{
    class Channel;
    class ChannelView;

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class SearchPopup : public BaseWindow
    {
    public:
        SearchPopup();

        void setChannel(std::shared_ptr<Channel> channel);

    private:
        void initLayout();
        void performSearch();

        LimitedQueueSnapshot<MessagePtr> snapshot_;
        QLineEdit* searchInput_;
        ChannelView* channelView_;
    };

}  // namespace chatterino
