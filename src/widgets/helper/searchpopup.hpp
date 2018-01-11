#pragma once

#include <memory>

#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "widgets/basewidget.hpp"

class QLineEdit;
namespace chatterino {
class Channel;
namespace widgets {
class ChannelView;

class SearchPopup : public BaseWidget
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
}
}
