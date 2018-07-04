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
    ChannelView *channelView = nullptr;
    ChannelPtr channel_ = Channel::getEmpty();

    QString userName;

    bool usedLogviewer = true;

    void initLayout();
    void setupView(std::vector<MessagePtr> messages);
    void getOverrustleLogs();
    void getLogviewerLogs();
};

}  // namespace chatterino
