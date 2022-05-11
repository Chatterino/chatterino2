#pragma once

#include "messages/MessageThread.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/splits/ReplyInput.hpp"
#include "widgets/splits/Split.hpp"

#include <memory>

namespace chatterino {

class ReplyThreadPopup final : public BaseWindow
{
    Q_OBJECT

public:
    ReplyThreadPopup(QWidget *parent, Split *split);

    void setThread(const std::shared_ptr<const MessageThread> &thread);

private:
    void addMessagesFromThread();

    // The message reply thread
    std::shared_ptr<const MessageThread> thread_;
    // The channel that the reply thread is in
    ChannelPtr channel_;
    Split *split_;

    struct {
        ChannelView *threadView = nullptr;
        ReplyInput *replyInput = nullptr;
    } ui_;
};

}  // namespace chatterino
