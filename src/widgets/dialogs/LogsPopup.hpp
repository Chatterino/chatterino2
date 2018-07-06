#pragma once

#include "common/Channel.hpp"
#include "widgets/BaseWindow.hpp"

namespace chatterino {

class Channel;
class ChannelView;

class LogsPopup : public BaseWindow
{
public:
    LogsPopup();

    void setInfo(std::shared_ptr<Channel> channel, QString userName);

private:
<<<<<<< HEAD
=======
    ChannelView *channelView_ = nullptr;
    ChannelPtr channel_ = Channel::getEmpty();

    QString userName_;
    int roomID_;

>>>>>>> Add channel badges and emotes to LogsPopup
    void initLayout();
    void setMessages(std::vector<MessagePtr> &messages);
    void getOverrustleLogs();
    void getLogviewerLogs();

    ChannelView *channelView_ = nullptr;
    ChannelPtr channel_ = Channel::getEmpty();

    QString userName_;
};

}  // namespace chatterino
