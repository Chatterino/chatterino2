#pragma once

#include "messages/MessageThread.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/splits/ReplyInput.hpp"
#include "widgets/splits/Split.hpp"

#include <boost/signals2.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>

#include <memory>

namespace chatterino {

class ReplyThreadPopup final : public BaseWindow
{
    Q_OBJECT

public:
    ReplyThreadPopup(bool closeAutomatically, QWidget *parent, Split *split);

    void setThread(const std::shared_ptr<const MessageThread> &thread);

private:
    void addMessagesFromThread();
    void updateInputUI();

    // The message reply thread
    std::shared_ptr<const MessageThread> thread_;
    // The channel that the reply thread is in
    ChannelPtr channel_;
    Split *split_;

    struct {
        ChannelView *threadView = nullptr;
        ReplyInput *replyInput = nullptr;
    } ui_;

    std::unique_ptr<pajlada::Signals::ScopedConnection> messageConnection_;
    std::vector<boost::signals2::scoped_connection> bSignals_;
};

}  // namespace chatterino
