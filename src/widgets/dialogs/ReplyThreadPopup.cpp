#include "ReplyThreadPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/MessageThread.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitInput.hpp"

const QString TEXT_TITLE("Reply Thread - @%1 in #%2");

namespace chatterino {

ReplyThreadPopup::ReplyThreadPopup(bool closeAutomatically, QWidget *parent,
                                   Split *split)
    : DraggablePopup(closeAutomatically, parent)
    , split_(split)
{
    this->setWindowTitle(QStringLiteral("Reply Thread"));
    this->setStayInScreenRect(true);

    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](std::vector<QString>) -> QString {
             this->deleteLater();
             return "";
         }},
        {"scrollPage",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.empty())
             {
                 qCWarning(chatterinoHotkeys)
                     << "scrollPage hotkey called without arguments!";
                 return "scrollPage hotkey called without arguments!";
             }
             auto direction = arguments.at(0);

             auto &scrollbar = this->ui_.threadView->getScrollBar();
             if (direction == "up")
             {
                 scrollbar.offset(-scrollbar.getLargeChange());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getLargeChange());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},

        // these actions make no sense in the context of a reply thread, so they aren't implemented
        {"execModeratorAction", nullptr},
        {"reject", nullptr},
        {"accept", nullptr},
        {"openTab", nullptr},
        {"search", nullptr},
    };

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    // initialize UI
    this->ui_.threadView =
        new ChannelView(this, this->split_, ChannelView::Context::ReplyThread);
    this->ui_.threadView->setMinimumSize(400, 100);
    this->ui_.threadView->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);
    this->ui_.threadView->mouseDown.connect([this](QMouseEvent *) {
        this->giveFocus(Qt::MouseFocusReason);
    });

    // Create SplitInput with inline replying disabled
    this->ui_.replyInput = new SplitInput(this, this->split_, false);

    this->bSignals_.emplace_back(
        getApp()->accounts->twitch.currentUserChanged.connect([this] {
            this->updateInputUI();
        }));

    layout->setSpacing(0);
    // provide draggable margin if frameless
    layout->setMargin(closeAutomatically ? 15 : 1);
    layout->addWidget(this->ui_.threadView, 1);
    layout->addWidget(this->ui_.replyInput);
}

void ReplyThreadPopup::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
    this->ui_.replyInput->setReply(this->thread_);
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
    // Bail out if not a twitch channel.
    // Special twitch channels will hide their reply input box.
    if (!channel || !channel->isTwitchChannel())
    {
        return;
    }

    this->ui_.replyInput->setVisible(channel->isWritable());

    auto user = getApp()->accounts->twitch.getCurrent();
    QString placeholderText;

    if (user->isAnon())
    {
        placeholderText = QStringLiteral("Log in to send messages...");
    }
    else
    {
        placeholderText =
            QStringLiteral("Reply as %1...")
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
