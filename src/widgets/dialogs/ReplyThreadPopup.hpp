#pragma once

#include "ForwardDecl.hpp"
#include "widgets/DraggablePopup.hpp"

#include <boost/signals2.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class MessageThread;
class Split;
class SplitInput;

class ReplyThreadPopup final : public DraggablePopup
{
    Q_OBJECT

public:
    ReplyThreadPopup(bool closeAutomatically, QWidget *parent, Split *split);

    void setThread(std::shared_ptr<MessageThread> thread);
    void giveFocus(Qt::FocusReason reason);

protected:
    void focusInEvent(QFocusEvent *event) override;

private:
    void addMessagesFromThread();
    void updateInputUI();

    // The message reply thread
    std::shared_ptr<MessageThread> thread_;
    // The channel that the reply thread is in
    ChannelPtr channel_;
    Split *split_;

    struct {
        ChannelView *threadView = nullptr;
        SplitInput *replyInput = nullptr;
    } ui_;

    std::unique_ptr<pajlada::Signals::ScopedConnection> messageConnection_;
    std::vector<boost::signals2::scoped_connection> bSignals_;
};

}  // namespace chatterino
