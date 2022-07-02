#include "ReplyThreadPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "messages/MessageThread.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/splits/ReplyInput.hpp"
#include "widgets/splits/Split.hpp"

const QString TEXT_TITLE("Reply Thread - @%1 in #%2");

namespace chatterino {

namespace {

// Duplicate of UserInfoPopup.cpp
#ifdef Q_OS_LINUX
    FlagsEnum<BaseWindow::Flags> popupFlags{BaseWindow::Dialog,
                                            BaseWindow::EnableCustomFrame};
    FlagsEnum<BaseWindow::Flags> popupFlagsCloseAutomatically{
        BaseWindow::EnableCustomFrame};
#else
    FlagsEnum<BaseWindow::Flags> popupFlags{BaseWindow::EnableCustomFrame};
    FlagsEnum<BaseWindow::Flags> popupFlagsCloseAutomatically{
        BaseWindow::EnableCustomFrame, BaseWindow::Frameless,
        BaseWindow::FramelessDraggable};
#endif

}  // namespace

ReplyThreadPopup::ReplyThreadPopup(bool closeAutomatically, QWidget *parent,
                                   Split *split)
    : BaseWindow(closeAutomatically ? popupFlagsCloseAutomatically : popupFlags,
                 parent)
    , split_(split)
{
    this->setWindowTitle("Reply Thread");
    this->setStayInScreenRect(true);

    if (closeAutomatically)
        this->setActionOnFocusLoss(BaseWindow::Delete);
    else
        this->setAttribute(Qt::WA_DeleteOnClose);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    // initialize UI
    this->ui_.threadView =
        new ChannelView(this, this->split_, ChannelView::Context::ReplyThread);
    this->ui_.threadView->setFloatingVisible(false);
    this->ui_.threadView->setMinimumSize(400, 100);
    this->ui_.threadView->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);
    this->ui_.threadView->mouseDown.connect([this](QMouseEvent *) {
        this->giveFocus(Qt::MouseFocusReason);
    });

    this->ui_.replyInput = new ReplyInput(this, this->split_);

    this->bSignals_.emplace_back(
        getApp()->accounts->twitch.currentUserChanged.connect([this] {
            this->updateInputUI();
        }));

    layout->setSpacing(0);
    layout->setMargin(1);
    layout->addWidget(this->ui_.threadView, 1);
    layout->addWidget(this->ui_.replyInput);
}

void ReplyThreadPopup::setThread(
    const std::shared_ptr<const MessageThread> &thread)
{
    this->thread_ = thread;
    this->ui_.replyInput->setThread(thread);
    this->addMessagesFromThread();
    this->updateInputUI();
}

void ReplyThreadPopup::addMessagesFromThread()
{
    this->ui_.threadView->clearMessages();
    if (!this->thread_)
    {
        return;
    }

    const auto &sourceChannel = this->split_->getChannel();
    this->setWindowTitle(TEXT_TITLE.arg(this->thread_->root()->loginName,
                                        sourceChannel->getName()));

    ChannelPtr virtualChannel;
    if (sourceChannel->isTwitchChannel())
    {
        virtualChannel =
            std::make_shared<TwitchChannel>(sourceChannel->getName());
    }
    else
    {
        virtualChannel = std::make_shared<Channel>(sourceChannel->getName(),
                                                   Channel::Type::None);
    }

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

    this->messageConnection_ =
        std::make_unique<pajlada::Signals::ScopedConnection>(
            sourceChannel->messageAppended.connect(
                [this, virtualChannel](MessagePtr &message, auto) {
                    if (message->replyThread == this->thread_)
                    {
                        // same reply thread, add message
                        virtualChannel->addMessage(message);
                    }
                }));
}

void ReplyThreadPopup::updateInputUI()
{
    auto channel = this->split_->getChannel();
    if (!channel || !channel->isTwitchChannel())
    {
        return;
    }

    this->ui_.replyInput->setVisible(channel->isWritable());

    auto user = getApp()->accounts->twitch.getCurrent();
    QString placeholderText;

    if (user->isAnon())
    {
        placeholderText = "Log in to send messages...";
    }
    else
    {
        placeholderText =
            QString("Reply as %1...")
                .arg(getApp()->accounts->twitch.getCurrent()->getUserName());
    }

    this->ui_.replyInput->setPlaceholderText(placeholderText);
}

void ReplyThreadPopup::giveFocus(Qt::FocusReason reason)
{
    this->ui_.replyInput->giveFocus(reason);
}

void ReplyThreadPopup::focusInEvent(QFocusEvent *event)
{
    this->giveFocus(event->reason());
}

}  // namespace chatterino
