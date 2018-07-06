#pragma once

#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "widgets/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino {

class Channel;
class ChannelView;

class SearchPopup : public BaseWindow
{
public:
    SearchPopup();

    void setChannel(std::shared_ptr<Channel> channel);

private:
    void initLayout();
    void performSearch();

    LimitedQueueSnapshot<MessagePtr> snapshot_;
    QLineEdit *searchInput_;
    ChannelView *channelView_;
};

}  // namespace chatterino
