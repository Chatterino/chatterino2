#include "widgets/dialogs/ReplyThreadPopup.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/Button.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/InvisibleSizeGrip.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitInput.hpp"

#include <QCheckBox>

const QString TEXT_TITLE("Reply Thread - @%1 in #%2");

namespace chatterino {

ReplyThreadPopup::ReplyThreadPopup(bool closeAutomatically, Split *split)
    : DraggablePopup(closeAutomatically, split)
    , split_(split)
{
    assert(split != nullptr);

    this->setWindowTitle(QStringLiteral("Reply Thread"));

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
                 scrollbar.offset(-scrollbar.getPageSize());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getPageSize());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},
        {"pin",
         [this](std::vector<QString> /*arguments*/) -> QString {
             this->togglePinned();
             return "";
         }},

        // these actions make no sense in the context of a reply thread, so they aren't implemented
        {"execModeratorAction", nullptr},
        {"reject", nullptr},
        {"accept", nullptr},
        {"openTab", nullptr},
        {"search", nullptr},
    };

    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);

    // initialize UI
    this->ui_.threadView =
        new ChannelView(this, this->split_, ChannelView::Context::ReplyThread);
    this->ui_.threadView->setMinimumSize(400, 100);
    this->ui_.threadView->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);
    // We can safely ignore this signal's connection since threadView will always be deleted before
    // the ReplyThreadPopup
    std::ignore =
        this->ui_.threadView->mouseDown.connect([this](QMouseEvent *) {
            this->giveFocus(Qt::MouseFocusReason);
        });

    // Create SplitInput with inline replying disabled
    this->ui_.replyInput =
        new SplitInput(this, this->split_, this->ui_.threadView, false);

    this->bSignals_.emplace_back(
        getApp()->getAccounts()->twitch.currentUserChanged.connect([this] {
            this->updateInputUI();
        }));

    // We can safely ignore this signal's connection since threadView will always be deleted before
    // the ReplyThreadPopup
    std::ignore = this->ui_.threadView->selectionChanged.connect([this]() {
        // clear SplitInput selection when selecting in ChannelView
        if (this->ui_.replyInput->hasSelection())
        {
            this->ui_.replyInput->clearSelection();
        }
    });

    // clear ChannelView selection when selecting in SplitInput
    // We can safely ignore this signal's connection since replyInput will always be deleted before
    // the ReplyThreadPopup
    std::ignore = this->ui_.replyInput->selectionChanged.connect([this]() {
        if (this->ui_.threadView->hasSelection())
        {
            this->ui_.threadView->clearSelection();
        }
    });

    auto layers = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QGridLayout>()
                      .withoutMargin();
    auto layout = layers.emplace<QVBoxLayout>();

    layout->setSpacing(0);
    // provide draggable margin if frameless
    auto marginPx = closeAutomatically ? 15 : 1;
    layout->setContentsMargins(marginPx, marginPx, marginPx, marginPx);

    // Top Row
    bool addCheckbox = getSettings()->enableThreadHighlight;
    if (addCheckbox || closeAutomatically)
    {
        auto *hbox = new QHBoxLayout();

        if (addCheckbox)
        {
            this->ui_.notificationCheckbox =
                new QCheckBox("Subscribe to thread", this);
            QObject::connect(this->ui_.notificationCheckbox,
                             &QCheckBox::toggled, [this](bool checked) {
                                 if (!this->thread_ ||
                                     this->thread_->subscribed() == checked)
                                 {
                                     return;
                                 }

                                 if (checked)
                                 {
                                     this->thread_->markSubscribed();
                                 }
                                 else
                                 {
                                     this->thread_->markUnsubscribed();
                                 }
                             });
            hbox->addWidget(this->ui_.notificationCheckbox);
            hbox->addStretch(1);
            this->ui_.notificationCheckbox->setFocusPolicy(Qt::NoFocus);
        }

        if (closeAutomatically)
        {
            hbox->addWidget(this->createPinButton(), 0, Qt::AlignRight);
            hbox->setContentsMargins(0, 0, 0, 5);
        }
        else
        {
            hbox->setContentsMargins(10, 0, 0, 4);
        }

        layout->addLayout(hbox, 1);
    }

    layout->addWidget(this->ui_.threadView, 1);
    layout->addWidget(this->ui_.replyInput);

    // size grip
    if (closeAutomatically)
    {
        layers->addWidget(new InvisibleSizeGrip(this), 0, 0,
                          Qt::AlignRight | Qt::AlignBottom);
    }
}

void ReplyThreadPopup::setThread(std::shared_ptr<MessageThread> thread)
{
    this->thread_ = std::move(thread);
    this->ui_.replyInput->setReply(this->thread_->root());
    this->addMessagesFromThread();
    this->updateInputUI();

    if (!this->thread_) [[unlikely]]
    {
        this->replySubscriptionSignal_ = boost::signals2::scoped_connection{};
        return;
    }

    auto updateCheckbox = [this]() {
        if (this->ui_.notificationCheckbox)
        {
            this->ui_.notificationCheckbox->setChecked(
                this->thread_->subscribed());
        }
    };
    updateCheckbox();

    this->replySubscriptionSignal_ =
        this->thread_->subscriptionUpdated.connect(updateCheckbox);
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

    if (sourceChannel->isTwitchChannel())
    {
        this->virtualChannel_ =
            std::make_shared<TwitchChannel>(sourceChannel->getName());
    }
    else
    {
        this->virtualChannel_ = std::make_shared<Channel>(
            sourceChannel->getName(), Channel::Type::None);
    }

    auto rootOverrideFlags =
        std::optional<MessageFlags>(this->thread_->root()->flags);
    rootOverrideFlags->set(MessageFlag::DoNotLog);

    this->virtualChannel_->addMessage(
        this->thread_->root(), MessageContext::Repost, rootOverrideFlags);
    for (const auto &msgRef : this->thread_->replies())
    {
        if (auto msg = msgRef.lock())
        {
            auto overrideFlags = std::optional<MessageFlags>(msg->flags);
            overrideFlags->set(MessageFlag::DoNotLog);

            this->virtualChannel_->addMessage(msg, MessageContext::Repost,
                                              overrideFlags);
        }
    }

    this->ui_.threadView->setChannel(this->virtualChannel_);
    this->ui_.threadView->setSourceChannel(sourceChannel);

    this->messageConnection_ =
        std::make_unique<pajlada::Signals::ScopedConnection>(
            sourceChannel->messageAppended.connect(
                [this](MessagePtr &message, auto) {
                    if (message->replyThread == this->thread_)
                    {
                        auto overrideFlags =
                            std::optional<MessageFlags>(message->flags);
                        overrideFlags->set(MessageFlag::DoNotLog);

                        // same reply thread, add message
                        this->virtualChannel_->addMessage(
                            message, MessageContext::Repost, overrideFlags);
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

    auto user = getApp()->getAccounts()->twitch.getCurrent();
    QString placeholderText;

    if (user->isAnon())
    {
        placeholderText = QStringLiteral("Log in to send messages...");
    }
    else
    {
        placeholderText = QStringLiteral("Reply as %1...")
                              .arg(getApp()
                                       ->getAccounts()
                                       ->twitch.getCurrent()
                                       ->getUserName());
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
