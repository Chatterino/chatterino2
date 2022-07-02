#pragma once

#include "ForwardDecl.hpp"
#include "widgets/DraggablePopup.hpp"

#include <boost/signals2.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class MessageThread;
class ReplyInput;
class Split;

class ReplyThreadPopup final : public DraggablePopup
{
    Q_OBJECT

public:
    ReplyThreadPopup(bool closeAutomatically, QWidget *parent, Split *split);

    void setThread(const std::shared_ptr<const MessageThread> &thread);
    void giveFocus(Qt::FocusReason reason);

protected:
    void focusInEvent(QFocusEvent *event) override;

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
