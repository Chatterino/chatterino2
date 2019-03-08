#pragma once

#if 0
#    include "ab/BaseWindow.hpp"

// TODO: fix
namespace chatterino
{
    class Channel;
    class ChannelView;

    class Channel;
    using ChannelPtr = std::shared_ptr<Channel>;

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    class LogsPopup : public ab::BaseWindow
    {
    public:
        LogsPopup();

        void setChannelName(QString channelName);
        void setChannel(std::shared_ptr<Channel> channel);
        void setTargetUserName(QString userName);

        void getLogs();

    private:
        ChannelView* channelView_ = nullptr;
        ChannelPtr channel_;

        QString userName_;
        QString channelName_;

        void initLayout();
        void setMessages(std::vector<MessagePtr>& messages);
        void getOverrustleLogs();
        void getLogviewerLogs(const QString& roomID);
    };
}  // namespace chatterino
#endif
