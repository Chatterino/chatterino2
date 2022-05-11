#include "ReplyThreadPopup.hpp"

#include "common/Channel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {

ReplyThreadPopup::ReplyThreadPopup(QWidget *parent, Split *split)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
    , split_(split)
{
    this->setWindowTitle("Reply Thread");
    this->setStayInScreenRect(true);
    this->setAttribute(Qt::WA_DeleteOnClose);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    // initialize UI
    this->ui_.threadView = new ChannelView(this, this->split_);
    this->ui_.threadView->setMinimumSize(400, 100);
    this->ui_.threadView->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);

    this->ui_.splitInput = new SplitInput(this->split_);

    layout->addWidget(this->ui_.threadView, 1);
    layout->addWidget(this->ui_.splitInput);
}

void ReplyThreadPopup::setThread(
    const std::shared_ptr<const MessageThread> &thread)
{
    this->thread_ = thread;
    this->addMessagesFromThread();
}

void ReplyThreadPopup::addMessagesFromThread()
{
    this->ui_.threadView->clearMessages();
    if (!this->thread_)
    {
        return;
    }

    const auto &sourceChannel = this->split_->getChannel();

    ChannelPtr virtualChannel(
        new Channel(sourceChannel->getName(), Channel::Type::None));
    this->ui_.threadView->setChannel(virtualChannel);
    this->ui_.threadView->setSourceChannel(sourceChannel);

    virtualChannel->addMessage(this->thread_->root());
    for (const auto &msgRef : this->thread_->replies())
    {
        if (auto msg = msgRef.lock())
        {
            virtualChannel->addMessage(msg);
        }
    }
}

}  // namespace chatterino
