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

    void setInfo(std::shared_ptr<Channel> channel, QString username);

private:
    ChannelView *channelView;
    ChannelPtr channel_ = nullptr;

    QString userName;
    QString channelName_;
    QString answer;

    QTime rustleTime;

    bool usedLogviewer = true;

    void initLayout();
    void setupView();
    void getOverrustleLogs();
    void getLogviewerLogs();
};

}  // namespace chatterino
