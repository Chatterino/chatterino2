#pragma once

#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "widgets/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino {

class Channel;

namespace widgets {

class ChannelView;

class SearchPopup : public BaseWindow
{
public:
    SearchPopup();

    void setChannel(std::shared_ptr<Channel> channel);

private:
    messages::LimitedQueueSnapshot<messages::MessagePtr> snapshot;
    QLineEdit *searchInput;
    ChannelView *channelView;

    void initLayout();
    void performSearch();
};

}  // namespace widgets
}  // namespace chatterino
