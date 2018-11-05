#include "ChannelView.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "debug/Benchmark.hpp"
#include "debug/Log.hpp"
#include "messages/Emote.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DistanceBetweenPoints.hpp"
#include "util/IncognitoBrowser.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/splits/Split.hpp"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsBlurEffect>
#include <QMessageBox>
#include <QPainter>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#define DRAW_WIDTH (this->width())
#define SELECTION_RESUME_SCROLLING_MSG_THRESHOLD 3
#define CHAT_HOVER_PAUSE_DURATION 1000

namespace chatterino {
namespace {
    void addEmoteContextMenuItems(const Emote &emote,
                                  MessageElementFlags creatorFlags, QMenu &menu)
    {
        auto openAction = menu.addAction("Open");
        auto openMenu = new QMenu;
        openAction->setMenu(openMenu);

        auto copyAction = menu.addAction("Copy");
        auto copyMenu = new QMenu;
        copyAction->setMenu(copyMenu);

        // see if the QMenu actually gets destroyed
        QObject::connect(openMenu, &QMenu::destroyed, [] {
            QMessageBox(QMessageBox::Information, "xD", "the menu got deleted")
                .exec();
        });

        // Add copy and open links for 1x, 2x, 3x
        auto addImageLink = [&](const ImagePtr &image, char scale) {
            if (!image->isEmpty())
            {
                copyMenu->addAction(
                    QString(scale) + "x link", [url = image->url()] {
                        QApplication::clipboard()->setText(url.string);
                    });
                openMenu->addAction(
                    QString(scale) + "x link", [url = image->url()] {
                        QDesktopServices::openUrl(QUrl(url.string));
                    });
            }
        };

        addImageLink(emote.images.getImage1(), '1');
        addImageLink(emote.images.getImage2(), '2');
        addImageLink(emote.images.getImage3(), '3');

        // Copy and open emote page link
        auto addPageLink = [&](const QString &name) {
            copyMenu->addSeparator();
            openMenu->addSeparator();

            copyMenu->addAction(
                "Copy " + name + " emote link", [url = emote.homePage] {
                    QApplication::clipboard()->setText(url.string);  //
                });
            openMenu->addAction(
                "Open " + name + " emote link", [url = emote.homePage] {
                    QDesktopServices::openUrl(QUrl(url.string));  //
                });
        };

        if (creatorFlags.has(MessageElementFlag::BttvEmote))
        {
            addPageLink("BTTV");
        }
        else if (creatorFlags.has(MessageElementFlag::FfzEmote))
        {
            addPageLink("FFZ");
        }
    }
}  // namespace

ChannelView::ChannelView(BaseWidget *parent)
    : BaseWidget(parent)
    , scrollBar_(new Scrollbar(this))
{
    this->setMouseTracking(true);

    this->initializeLayout();
    this->initializeScrollbar();
    this->initializeSignals();

    this->pauseTimer_.setSingleShot(true);
    QObject::connect(&this->pauseTimer_, &QTimer::timeout, this, [this] {
        /// remove elements that are finite
        for (auto it = this->pauses_.begin(); it != this->pauses_.end();)
            it = it->second ? this->pauses_.erase(it) : ++it;

        this->updatePauseTimer();
    });

    auto shortcut = new QShortcut(QKeySequence("Ctrl+C"), this);
    QObject::connect(shortcut, &QShortcut::activated, [this] {
        QGuiApplication::clipboard()->setText(this->getSelectedText());
    });

    this->clickTimer_ = new QTimer(this);
    this->clickTimer_->setSingleShot(true);
    this->clickTimer_->setInterval(500);
}

void ChannelView::initializeLayout()
{
    this->goToBottom_ = new EffectLabel(this, 0);
    this->goToBottom_->setStyleSheet(
        "background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom_->getLabel().setText("More messages below");
    this->goToBottom_->setVisible(false);

    QObject::connect(this->goToBottom_, &EffectLabel::leftClicked, this, [=] {
        QTimer::singleShot(180, [=] {
            this->scrollBar_->scrollToBottom(
                getSettings()->enableSmoothScrollingNewMessages.getValue());
        });
    });
}

void ChannelView::initializeScrollbar()
{
    this->scrollBar_->getCurrentValueChanged().connect([this] {
        this->performLayout(true);
        this->queueUpdate();
    });
}

void ChannelView::initializeSignals()
{
    this->connections_.push_back(
        getApp()->windows->wordFlagsChanged.connect([this] {
            this->queueLayout();
            this->update();
        }));

    getSettings()->showLastMessageIndicator.connect(
        [this](auto, auto) { this->update(); }, this->connections_);

    connections_.push_back(
        getApp()->windows->repaintGifs.connect([&] { this->queueUpdate(); }));

    connections_.push_back(
        getApp()->windows->layout.connect([&](Channel *channel) {
            if (channel == nullptr || this->channel_.get() == channel)
                this->queueLayout();
        }));

    connections_.push_back(
        getApp()->fonts->fontChanged.connect([this] { this->queueLayout(); }));
}

bool ChannelView::paused() const
{
    /// No elements in the map -> not paused
    return !this->pauses_.empty();
}

void ChannelView::pause(PauseReason reason, boost::optional<uint> msecs)
{
    if (msecs)
    {
        /// Msecs has a value
        auto timePoint =
            SteadyClock::now() + std::chrono::milliseconds(msecs.get());
        auto it = this->pauses_.find(reason);

        if (it == this->pauses_.end())
        {
            /// No value found so we insert a new one.
            this->pauses_[reason] = timePoint;
        }
        else
        {
            /// If the new time point is newer then we override.
            if (it->second && it->second.get() < timePoint)
                it->second = timePoint;
        }
    }
    else
    {
        /// Msecs is none -> pause is infinite.
        /// We just override the value.
        this->pauses_[reason] = boost::none;
    }

    this->updatePauseTimer();
}

void ChannelView::unpause(PauseReason reason)
{
    /// Remove the value from the map
    this->pauses_.erase(reason);

    this->updatePauseTimer();
}

void ChannelView::updatePauseTimer()
{
    using namespace std::chrono;

    if (this->pauses_.empty())
    {
        /// No pauses so we can stop the timer
        this->pauseEnd = boost::none;
        this->pauseTimer_.stop();

        this->queueLayout();
    }
    else if (std::any_of(this->pauses_.begin(), this->pauses_.end(),
                         [](auto &&value) { return !value.second; }))
    {
        /// Some of the pauses are infinite
        this->pauseEnd = boost::none;
        this->pauseTimer_.stop();
    }
    else
    {
        /// Get the maximum pause
        auto max = std::max_element(
                       this->pauses_.begin(), this->pauses_.end(),
                       [](auto &&a, auto &&b) { return a.second > b.second; })
                       ->second.get();

        if (max != this->pauseEnd)
        {
            /// Start the timer
            this->pauseEnd = max;
            this->pauseTimer_.start(
                duration_cast<milliseconds>(max - SteadyClock::now()));
        }
    }
}

void ChannelView::themeChangedEvent()
{
    BaseWidget::themeChangedEvent();

    this->queueLayout();
}

void ChannelView::queueUpdate()
{
    //    if (this->updateTimer.isActive()) {
    //        this->updateQueued = true;
    //        return;
    //    }

    //    this->repaint();
    this->update();

    //    this->updateTimer.start();
}

void ChannelView::queueLayout()
{
    //    if (!this->layoutCooldown->isActive()) {
    this->performLayout();

    //        this->layoutCooldown->start();
    //    } else {
    //        this->layoutQueued = true;
    //    }
}

void ChannelView::performLayout(bool causedByScrollbar)
{
    // BenchmarkGuard benchmark("layout");

    /// Get messages and check if there are at least 1
    auto messages = this->getMessagesSnapshot();

    this->showingLatestMessages_ =
        this->scrollBar_->isAtBottom() || !this->scrollBar_->isVisible();

    /// Layout visible messages
    this->layoutVisibleMessages(messages);

    /// Update scrollbar
    this->updateScrollbar(messages, causedByScrollbar);

    this->goToBottom_->setVisible(this->enableScrollingToBottom_ &&
                                  this->scrollBar_->isVisible() &&
                                  !this->scrollBar_->isAtBottom());
}

void ChannelView::layoutVisibleMessages(
    LimitedQueueSnapshot<MessageLayoutPtr> &messages)
{
    const auto start = size_t(this->scrollBar_->getCurrentValue());
    const auto layoutWidth = this->getLayoutWidth();
    const auto flags = this->getFlags();
    auto redrawRequired = false;

    if (messages.size() > start)
    {
        auto y = int(-(messages[start]->getHeight() *
                       (fmod(this->scrollBar_->getCurrentValue(), 1))));

        for (auto i = start; i < messages.size() && y >= this->height(); i++)
        {
            auto message = messages[i];

            redrawRequired |=
                message->layout(layoutWidth, this->getScale(), flags);

            y += message->getHeight();
        }
    }

    if (redrawRequired)
        this->queueUpdate();
}

void ChannelView::updateScrollbar(
    LimitedQueueSnapshot<MessageLayoutPtr> &messages, bool causedByScrollbar)
{
    if (messages.size() == 0)
    {
        this->scrollBar_->setVisible(false);
        return;
    }

    /// Layout the messages at the bottom
    auto h = this->height() - 8;
    auto flags = this->getFlags();
    auto layoutWidth = this->getLayoutWidth();
    auto showScrollbar = false;

    // convert i to int since it checks >= 0
    for (auto i = int(messages.size()) - 1; i >= 0; i--)
    {
        auto *message = messages[i].get();

        message->layout(layoutWidth, this->getScale(), flags);

        h -= message->getHeight();

        if (h < 0)  // break condition
        {
            this->scrollBar_->setLargeChange((messages.size() - i) +
                                             qreal(h) / message->getHeight());

            showScrollbar = true;
            break;
        }
    }

    /// Update scrollbar values
    this->scrollBar_->setVisible(showScrollbar);

    if (!showScrollbar && !causedByScrollbar)
    {
        this->scrollBar_->setDesiredValue(0);
    }

    this->scrollBar_->setMaximum(messages.size());

    // If we were showing the latest messages and the scrollbar now wants to be
    // rendered, scroll to bottom
    if (this->enableScrollingToBottom_ && this->showingLatestMessages_ &&
        showScrollbar)
    {
        this->scrollBar_->scrollToBottom(
            // this->messageWasAdded &&
            getSettings()->enableSmoothScrollingNewMessages.getValue());
        this->messageWasAdded_ = false;
    }
}

void ChannelView::clearMessages()
{
    // Clear all stored messages in this chat widget
    this->messages.clear();
    this->scrollBar_->clearHighlights();

    // Layout chat widget messages, and force an update regardless if there are
    // no messages
    this->queueLayout();
    this->queueUpdate();
}

Scrollbar &ChannelView::getScrollBar()
{
    return *this->scrollBar_;
}

QString ChannelView::getSelectedText()
{
    QString result = "";

    LimitedQueueSnapshot<MessageLayoutPtr> messagesSnapshot =
        this->getMessagesSnapshot();

    Selection _selection = this->selection_;

    if (_selection.isEmpty())
    {
        return result;
    }

    for (int msg = _selection.selectionMin.messageIndex;
         msg <= _selection.selectionMax.messageIndex; msg++)
    {
        MessageLayoutPtr layout = messagesSnapshot[msg];
        int from = msg == _selection.selectionMin.messageIndex
                       ? _selection.selectionMin.charIndex
                       : 0;
        int to = msg == _selection.selectionMax.messageIndex
                     ? _selection.selectionMax.charIndex
                     : layout->getLastCharacterIndex() + 1;

        layout->addSelectionText(result, from, to);
    }

    return result;
}

bool ChannelView::hasSelection()
{
    return !this->selection_.isEmpty();
}

void ChannelView::clearSelection()
{
    this->selection_ = Selection();
    queueLayout();
}

void ChannelView::setEnableScrollingToBottom(bool value)
{
    this->enableScrollingToBottom_ = value;
}

bool ChannelView::getEnableScrollingToBottom() const
{
    return this->enableScrollingToBottom_;
}

void ChannelView::setOverrideFlags(boost::optional<MessageElementFlags> value)
{
    this->overrideFlags_ = value;
}

const boost::optional<MessageElementFlags> &ChannelView::getOverrideFlags()
    const
{
    return this->overrideFlags_;
}

LimitedQueueSnapshot<MessageLayoutPtr> ChannelView::getMessagesSnapshot()
{
    if (!this->paused() /*|| this->scrollBar_->isVisible()*/)
    {
        this->snapshot_ = this->messages.getSnapshot();
    }

    return this->snapshot_;
}

void ChannelView::setChannel(ChannelPtr newChannel)
{
    if (this->channel_)
    {
        this->detachChannel();
    }

    this->clearMessages();

    // on new message
    this->channelConnections_.push_back(newChannel->messageAppended.connect(
        [this](MessagePtr &message,
               boost::optional<MessageFlags> overridingFlags) {
            this->messageAppended(message, overridingFlags);
        }));

    this->channelConnections_.push_back(
        newChannel->messagesAddedAtStart.connect(
            [this](std::vector<MessagePtr> &messages) {
                this->messageAddedAtStart(messages);
            }));

    // on message removed
    this->channelConnections_.push_back(
        newChannel->messageRemovedFromStart.connect(
            [this](MessagePtr &message) {
                this->messageRemoveFromStart(message);
            }));

    // on message replaced
    this->channelConnections_.push_back(newChannel->messageReplaced.connect(
        [this](size_t index, MessagePtr replacement) {
            this->messageReplaced(index, replacement);
        }));

    auto snapshot = newChannel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.size(); i++)
    {
        MessageLayoutPtr deleted;

        auto messageRef = new MessageLayout(snapshot[i]);

        if (this->lastMessageHasAlternateBackground_)
        {
            messageRef->flags.set(MessageLayoutFlag::AlternateBackground);
        }
        this->lastMessageHasAlternateBackground_ =
            !this->lastMessageHasAlternateBackground_;

        this->messages.pushBack(MessageLayoutPtr(messageRef), deleted);
    }

    this->channel_ = newChannel;

    this->queueLayout();
    this->queueUpdate();

    // Notifications
    if (auto tc = dynamic_cast<TwitchChannel *>(newChannel.get()))
    {
        tc->liveStatusChanged.connect([this]() {
            this->liveStatusChanged.invoke();  //
        });
    }
}

void ChannelView::messageAppended(MessagePtr &message,
                                  boost::optional<MessageFlags> overridingFlags)
{
    MessageLayoutPtr deleted;

    auto *messageFlags = &message->flags;
    if (overridingFlags)
    {
        messageFlags = overridingFlags.get_ptr();
    }

    auto messageRef = new MessageLayout(message);

    if (this->lastMessageHasAlternateBackground_)
    {
        messageRef->flags.set(MessageLayoutFlag::AlternateBackground);
    }
    if (this->channel_->shouldIgnoreHighlights())
    {
        messageRef->flags.set(MessageLayoutFlag::IgnoreHighlights);
    }
    this->lastMessageHasAlternateBackground_ =
        !this->lastMessageHasAlternateBackground_;

    if (this->messages.pushBack(MessageLayoutPtr(messageRef), deleted))
    {
        //                if (!this->isPaused()) {
        if (this->scrollBar_->isAtBottom())
        {
            this->scrollBar_->scrollToBottom();
        }
        else
        {
            this->scrollBar_->offset(-1);
        }
        //                }
    }

    if (!messageFlags->has(MessageFlag::DoNotTriggerNotification))
    {
        if (messageFlags->has(MessageFlag::Highlighted))
        {
            this->tabHighlightRequested.invoke(HighlightState::Highlighted);
        }
        else
        {
            this->tabHighlightRequested.invoke(HighlightState::NewMessage);
        }
    }

    if (this->channel_->getType() != Channel::Type::TwitchMentions)
    {
        this->scrollBar_->addHighlight(message->getScrollBarHighlight());
    }

    this->messageWasAdded_ = true;
    this->queueLayout();
}

void ChannelView::messageAddedAtStart(std::vector<MessagePtr> &messages)
{
    std::vector<MessageLayoutPtr> messageRefs;
    messageRefs.resize(messages.size());

    /// Create message layouts
    for (size_t i = 0; i < messages.size(); i++)
    {
        auto layout = new MessageLayout(messages.at(i));

        // alternate color
        if (!this->lastMessageHasAlternateBackgroundReverse_)
            layout->flags.set(MessageLayoutFlag::AlternateBackground);
        this->lastMessageHasAlternateBackgroundReverse_ =
            !this->lastMessageHasAlternateBackgroundReverse_;

        messageRefs.at(i) = MessageLayoutPtr(layout);
    }

    /// Add the messages at the start
    if (this->messages.pushFront(messageRefs).size() > 0)
    {
        if (this->scrollBar_->isAtBottom())
            this->scrollBar_->scrollToBottom();
        else
            this->scrollBar_->offset(qreal(messages.size()));
    }

    /// Add highlights
    std::vector<ScrollbarHighlight> highlights;
    highlights.reserve(messages.size());
    for (size_t i = 0; i < messages.size(); i++)
    {
        highlights.push_back(messages.at(i)->getScrollBarHighlight());
    }

    this->scrollBar_->addHighlightsAtStart(highlights);

    this->messageWasAdded_ = true;
    this->queueLayout();
}

void ChannelView::messageRemoveFromStart(MessagePtr &message)
{
    this->selection_.selectionMin.messageIndex--;
    this->selection_.selectionMax.messageIndex--;
    this->selection_.start.messageIndex--;
    this->selection_.end.messageIndex--;

    this->queueLayout();
}

void ChannelView::messageReplaced(size_t index, MessagePtr &replacement)
{
    if (index >= this->messages.getSnapshot().size() || index < 0)
    {
        return;
    }

    MessageLayoutPtr newItem(new MessageLayout(replacement));
    auto snapshot = this->messages.getSnapshot();
    if (index >= snapshot.size())
    {
        log("Tried to replace out of bounds message. Index: {}. "
            "Length: {}",
            index, snapshot.size());
        return;
    }

    const auto &message = snapshot[index];
    if (message->flags.has(MessageLayoutFlag::AlternateBackground))
    {
        newItem->flags.set(MessageLayoutFlag::AlternateBackground);
    }

    this->scrollBar_->replaceHighlight(index,
                                       replacement->getScrollBarHighlight());

    this->messages.replaceItem(message, newItem);
    this->queueLayout();
}

void ChannelView::detachChannel()
{
    this->channelConnections_.clear();
}

void ChannelView::updateLastReadMessage()
{
    auto _snapshot = this->getMessagesSnapshot();

    if (_snapshot.size() > 0)
    {
        this->lastReadMessage_ = _snapshot[_snapshot.size() - 1];
    }

    this->update();
}

void ChannelView::resizeEvent(QResizeEvent *)
{
    this->scrollBar_->setGeometry(this->width() - this->scrollBar_->width(), 0,
                                  this->scrollBar_->width(), this->height());

    this->goToBottom_->setGeometry(0, this->height() - 32, this->width(), 32);

    this->scrollBar_->raise();

    this->queueLayout();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start,
                               const SelectionItem &end)
{
    // selections
    if (!this->selecting_ && start != end)
    {
        // this->messagesAddedSinceSelectionPause_ = 0;

        this->selecting_ = true;
        // this->pausedBySelection_ = true;
    }

    this->selection_ = Selection(start, end);

    this->selectionChanged.invoke();
}

MessageElementFlags ChannelView::getFlags() const
{
    auto app = getApp();

    if (this->overrideFlags_)
    {
        return this->overrideFlags_.get();
    }

    MessageElementFlags flags = app->windows->getWordFlags();

    Split *split = dynamic_cast<Split *>(this->parentWidget());

    if (split != nullptr)
    {
        if (split->getModerationMode())
        {
            flags.set(MessageElementFlag::ModeratorTools);
        }
        if (this->channel_ == app->twitch.server->mentionsChannel)
        {
            flags.set(MessageElementFlag::ChannelName);
        }
    }

    return flags;
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BenchmarkGuard benchmark("paint");

    QPainter painter(this);

    painter.fillRect(rect(), this->theme->splits.background);

    // draw messages
    this->drawMessages(painter);

    // draw paused sign
    if (this->paused())
    {
        auto a = this->getScale() * 16;
        auto brush = QBrush(QColor(127, 127, 127, 63));
        painter.fillRect(QRectF(this->width() - a, a / 4, a / 4, a), brush);
        painter.fillRect(QRectF(this->width() - a / 2, a / 4, a / 4, a), brush);
    }
}

// if overlays is false then it draws the message, if true then it draws things
// such as the grey overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = size_t(this->scrollBar_->getCurrentValue());

    if (start >= messagesSnapshot.size())
    {
        return;
    }

    int y = int(-(messagesSnapshot[start].get()->getHeight() *
                  (fmod(this->scrollBar_->getCurrentValue(), 1))));

    MessageLayout *end = nullptr;
    bool windowFocused = this->window() == QApplication::activeWindow();

    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        MessageLayout *layout = messagesSnapshot[i].get();

        bool isLastMessage = false;
        if (getSettings()->showLastMessageIndicator)
        {
            isLastMessage = this->lastReadMessage_.get() == layout;
        }

        layout->paint(painter, DRAW_WIDTH, y, i, this->selection_,
                      isLastMessage, windowFocused);

        y += layout->getHeight();

        end = layout;
        if (y > this->height())
        {
            break;
        }
    }

    if (end == nullptr)
    {
        return;
    }

    // remove messages that are on screen
    // the messages that are left at the end get their buffers reset
    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        auto it = this->messagesOnScreen_.find(messagesSnapshot[i]);
        if (it != this->messagesOnScreen_.end())
        {
            this->messagesOnScreen_.erase(it);
        }
    }

    // delete the message buffers that aren't on screen
    for (const std::shared_ptr<MessageLayout> &item : this->messagesOnScreen_)
    {
        item->deleteBuffer();
    }

    this->messagesOnScreen_.clear();

    // add all messages on screen to the map
    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        std::shared_ptr<MessageLayout> layout = messagesSnapshot[i];

        this->messagesOnScreen_.insert(layout);

        if (layout.get() == end)
        {
            break;
        }
    }
}

void ChannelView::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() != Qt::Vertical)
        return;

    if (event->modifiers() & Qt::ControlModifier)
    {
        event->ignore();
        return;
    }

    if (this->scrollBar_->isVisible())
    {
        float mouseMultiplier = getSettings()->mouseScrollMultiplier;

        qreal desired = this->scrollBar_->getDesiredValue();
        qreal delta = event->delta() * qreal(1.5) * mouseMultiplier;

        auto snapshot = this->getMessagesSnapshot();
        int snapshotLength = int(snapshot.size());
        int i = std::min<int>(int(desired), snapshotLength);

        if (delta > 0)
        {
            qreal scrollFactor = fmod(desired, 1);
            qreal currentScrollLeft =
                int(scrollFactor * snapshot[i]->getHeight());

            for (; i >= 0; i--)
            {
                if (delta < currentScrollLeft)
                {
                    desired -= scrollFactor * (delta / currentScrollLeft);
                    break;
                }
                else
                {
                    delta -= currentScrollLeft;
                    desired -= scrollFactor;
                }

                if (i == 0)
                {
                    desired = 0;
                }
                else
                {
                    snapshot[i - 1]->layout(this->getLayoutWidth(),
                                            this->getScale(), this->getFlags());
                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i - 1]->getHeight();
                }
            }
        }
        else
        {
            delta = -delta;
            qreal scrollFactor = 1 - fmod(desired, 1);
            qreal currentScrollLeft =
                int(scrollFactor * snapshot[i]->getHeight());

            for (; i < snapshotLength; i++)
            {
                if (delta < currentScrollLeft)
                {
                    desired +=
                        scrollFactor * (qreal(delta) / currentScrollLeft);
                    break;
                }
                else
                {
                    delta -= currentScrollLeft;
                    desired += scrollFactor;
                }

                if (i == snapshotLength - 1)
                {
                    desired = snapshot.size();
                }
                else
                {
                    snapshot[i + 1]->layout(this->getLayoutWidth(),
                                            this->getScale(), this->getFlags());

                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i + 1]->getHeight();
                }
            }
        }

        this->scrollBar_->setDesiredValue(desired, true);
    }
}

void ChannelView::enterEvent(QEvent *)
{
}

void ChannelView::leaveEvent(QEvent *)
{
    this->unpause(PauseReason::Mouse);

    this->queueLayout();
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier))
    {
        this->unsetCursor();

        event->ignore();
        return;
    }

    /// Pause on hover
    if (getSettings()->pauseChatOnHover.getValue())
    {
        this->pause(PauseReason::Mouse, 500);
    }

    auto tooltipWidget = TooltipWidget::getInstance();
    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    // is selecting
    if (this->isMouseDown_)
    {
        // this->pause(PauseReason::Selecting, 300);
        int index = layout->getSelectionIndex(relativePos);

        this->setSelection(this->selection_.start,
                           SelectionItem(messageIndex, index));

        this->queueUpdate();
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        this->setCursor(Qt::PointingHandCursor);
        tooltipWidget->hide();
        return;
    }

    // check if word underneath cursor
    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr)
    {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    if (this->isDoubleClick_)
    {
        int wordStart;
        int wordEnd;
        this->getWordBounds(layout.get(), hoverLayoutElement, relativePos,
                            wordStart, wordEnd);
        SelectionItem newStart(messageIndex, wordStart);
        SelectionItem newEnd(messageIndex, wordEnd);

        // Selection changed in same message
        if (messageIndex == this->dCSelection_.origMessageIndex)
        {
            // Selecting to the left
            if (wordStart < this->selection_.start.charIndex &&
                !this->dCSelection_.selectingRight)
            {
                this->dCSelection_.selectingLeft = true;
                // Ensure that the original word stays selected(Edge case)
                if (wordStart > this->dCSelection_.originalEnd)
                {
                    this->setSelection(this->dCSelection_.origStartItem,
                                       newEnd);
                }
                else
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                // Selecting to the right
            }
            else if (wordEnd > this->selection_.end.charIndex &&
                     !this->dCSelection_.selectingLeft)
            {
                this->dCSelection_.selectingRight = true;
                // Ensure that the original word stays selected(Edge case)
                if (wordEnd < this->dCSelection_.originalStart)
                {
                    this->setSelection(newStart,
                                       this->dCSelection_.origEndItem);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
            // Swapping from selecting left to selecting right
            if (wordStart > this->selection_.start.charIndex &&
                !this->dCSelection_.selectingRight)
            {
                if (wordStart > this->dCSelection_.originalEnd)
                {
                    this->dCSelection_.selectingLeft = false;
                    this->dCSelection_.selectingRight = true;
                    this->setSelection(this->dCSelection_.origStartItem,
                                       newEnd);
                }
                else
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                // Swapping from selecting right to selecting left
            }
            else if (wordEnd < this->selection_.end.charIndex &&
                     !this->dCSelection_.selectingLeft)
            {
                if (wordEnd < this->dCSelection_.originalStart)
                {
                    this->dCSelection_.selectingLeft = true;
                    this->dCSelection_.selectingRight = false;
                    this->setSelection(newStart,
                                       this->dCSelection_.origEndItem);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
            // Selection changed in a different message
        }
        else
        {
            // Message over the original
            if (messageIndex < this->selection_.start.messageIndex)
            {
                // Swapping from left to right selecting
                if (!this->dCSelection_.selectingLeft)
                {
                    this->dCSelection_.selectingLeft = true;
                    this->dCSelection_.selectingRight = false;
                }
                if (wordStart < this->selection_.start.charIndex &&
                    !this->dCSelection_.selectingRight)
                {
                    this->dCSelection_.selectingLeft = true;
                }
                this->setSelection(newStart, this->dCSelection_.origEndItem);
                // Message under the original
            }
            else if (messageIndex > this->selection_.end.messageIndex)
            {
                // Swapping from right to left selecting
                if (!this->dCSelection_.selectingRight)
                {
                    this->dCSelection_.selectingLeft = false;
                    this->dCSelection_.selectingRight = true;
                }
                if (wordEnd > this->selection_.end.charIndex &&
                    !this->dCSelection_.selectingLeft)
                {
                    this->dCSelection_.selectingRight = true;
                }
                this->setSelection(this->dCSelection_.origStartItem, newEnd);
                // Selection changed in non original message
            }
            else
            {
                if (this->dCSelection_.selectingLeft)
                {
                    this->setSelection(newStart, this->selection_.end);
                }
                else
                {
                    this->setSelection(this->selection_.start, newEnd);
                }
            }
        }
        // Reset direction of selection
        if (wordStart == this->dCSelection_.originalStart &&
            wordEnd == this->dCSelection_.originalEnd)
        {
            this->dCSelection_.selectingLeft =
                this->dCSelection_.selectingRight = false;
        }
    }

    const auto &tooltip = hoverLayoutElement->getCreator().getTooltip();
    bool isLinkValid = hoverLayoutElement->getLink().isValid();

    if (tooltip.isEmpty())
    {
        tooltipWidget->hide();
    }
    else if (isLinkValid && !getSettings()->linkInfoTooltip)
    {
        tooltipWidget->hide();
    }
    else
    {
        tooltipWidget->moveTo(this, event->globalPos());
        tooltipWidget->setWordWrap(isLinkValid);
        tooltipWidget->setText(tooltip);
        tooltipWidget->adjustSize();
        tooltipWidget->show();
        tooltipWidget->raise();
    }

    // check if word has a link
    if (isLinkValid)
    {
        this->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        this->setCursor(Qt::ArrowCursor);
    }
}

void ChannelView::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown.invoke(event);

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        setCursor(Qt::ArrowCursor);
        auto messagesSnapshot = this->getMessagesSnapshot();
        if (messagesSnapshot.size() == 0)
        {
            return;
        }

        // Start selection at the last message at its last index
        if (event->button() == Qt::LeftButton)
        {
            auto lastMessageIndex = messagesSnapshot.size() - 1;
            auto lastMessage = messagesSnapshot[lastMessageIndex];
            auto lastCharacterIndex = lastMessage->getLastCharacterIndex();

            SelectionItem selectionItem(lastMessageIndex, lastCharacterIndex);
            this->setSelection(selectionItem, selectionItem);
        }
        return;
    }

    // check if message is collapsed
    switch (event->button())
    {
        case Qt::LeftButton:
        {
            this->lastPressPosition_ = event->screenPos();
            this->isMouseDown_ = true;

            if (layout->flags.has(MessageLayoutFlag::Collapsed))
                return;

            if (getSettings()->linksDoubleClickOnly.getValue())
            {
                this->pause(PauseReason::DoubleClick, 200);
            }

            int index = layout->getSelectionIndex(relativePos);
            auto selectionItem = SelectionItem(messageIndex, index);
            this->setSelection(selectionItem, selectionItem);
        }
        break;

        case Qt::RightButton:
        {
            this->lastRightPressPosition_ = event->screenPos();
            this->isRightMouseDown_ = true;
        }
        break;

        default:;
    }

    this->update();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    // check if mouse was pressed
    if (event->button() == Qt::LeftButton)
    {
        this->dCSelection_.selectingLeft = this->dCSelection_.selectingRight =
            false;
        if (this->isDoubleClick_)
        {
            this->isDoubleClick_ = false;
            // Was actually not a wanted triple-click
            if (fabsf(distanceBetweenPoints(this->lastDClickPosition_,
                                            event->screenPos())) > 10.f)
            {
                this->clickTimer_->stop();
                return;
            }
        }
        else if (this->isMouseDown_)
        {
            this->isMouseDown_ = false;

            if (fabsf(distanceBetweenPoints(this->lastPressPosition_,
                                            event->screenPos())) > 15.f)
            {
                return;
            }
        }
        else
        {
            return;
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (this->isRightMouseDown_)
        {
            this->isRightMouseDown_ = false;

            if (fabsf(distanceBetweenPoints(this->lastRightPressPosition_,
                                            event->screenPos())) > 15.f)
            {
                return;
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        // not left or right button
        return;
    }
    // find message
    this->queueLayout();

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message found
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        // No message at clicked position
        return;
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        layout->flags.set(MessageLayoutFlag::Expanded);
        layout->flags.set(MessageLayoutFlag::RequiresLayout);

        this->queueLayout();
        return;
    }

    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);
    // Triple-clicking a message selects the whole message
    if (this->clickTimer_->isActive() && this->selecting_)
    {
        if (fabsf(distanceBetweenPoints(this->lastDClickPosition_,
                                        event->screenPos())) < 10.f)
        {
            this->selectWholeMessage(layout.get(), messageIndex);
        }
    }

    if (hoverLayoutElement == nullptr)
    {
        return;
    }

    // handle the click
    this->handleMouseClick(event, hoverLayoutElement, layout.get());
}

void ChannelView::handleMouseClick(QMouseEvent *event,
                                   const MessageLayoutElement *hoveredElement,
                                   MessageLayout *layout)
{
    switch (event->button())
    {
        case Qt::LeftButton:
        {
            if (this->selecting_)
            {
                // this->pausedBySelection = false;
                this->selecting_ = false;
                // this->pauseTimeout.stop();
                // this->pausedTemporarily = false;

                this->queueLayout();
            }

            auto &link = hoveredElement->getLink();
            if (!getSettings()->linksDoubleClickOnly)
            {
                this->handleLinkClick(event, link, layout);
            }

            // Invoke to signal from EmotePopup.
            if (link.type == Link::InsertText)
            {
                this->linkClicked.invoke(link);
            }
        }
        break;
        case Qt::RightButton:
        {
            auto insertText = [=](QString text) {
                if (auto split = dynamic_cast<Split *>(this->parentWidget()))
                {
                    split->insertTextToInput(text);
                }
            };

            auto &link = hoveredElement->getLink();
            if (link.type == Link::UserInfo)
            {
                insertText("@" + link.value + ", ");
            }
            else if (link.type == Link::UserWhisper)
            {
                insertText("/w " + link.value + " ");
            }
            else
            {
                this->addContextMenuItems(hoveredElement, layout);
            }
        }
        break;
        default:;
    }
}

void ChannelView::addContextMenuItems(
    const MessageLayoutElement *hoveredElement, MessageLayout *layout)
{
    const auto &creator = hoveredElement->getCreator();
    auto creatorFlags = creator.getFlags();

    static QMenu *menu = new QMenu;
    menu->clear();

    // Emote actions
    if (creatorFlags.hasAny(
            {MessageElementFlag::EmoteImages, MessageElementFlag::EmojiImage}))
    {
        const auto emoteElement = dynamic_cast<const EmoteElement *>(&creator);
        if (emoteElement)
            addEmoteContextMenuItems(*emoteElement->getEmote(), creatorFlags,
                                     *menu);
    }

    // add seperator
    if (!menu->actions().empty())
    {
        menu->addSeparator();
    }

    // Link copy
    if (hoveredElement->getLink().type == Link::Url)
    {
        QString url = hoveredElement->getLink().value;

        // open link
        bool incognitoByDefault = supportsIncognitoLinks() &&
                                  layout->getMessage()->loginName == "hemirt";
        menu->addAction("Open link", [url, incognitoByDefault] {
            if (incognitoByDefault)
                openLinkIncognito(url);
            else
                QDesktopServices::openUrl(QUrl(url));
        });
        // open link default
        if (supportsIncognitoLinks())
        {
            menu->addAction("Open link incognito",
                            [url] { openLinkIncognito(url); });
        }
        menu->addAction("Copy link",
                        [url] { QApplication::clipboard()->setText(url); });

        menu->addSeparator();
    }

    // Copy actions
    if (!this->selection_.isEmpty())
    {
        menu->addAction("Copy selection", [this] {
            QGuiApplication::clipboard()->setText(this->getSelectedText());
        });
    }

    menu->addAction("Copy message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString, 0, INT_MAX,
                                 CopyMode::OnlyTextAndEmotes);

        QGuiApplication::clipboard()->setText(copyString);
    });

    menu->addAction("Copy full message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString);

        QGuiApplication::clipboard()->setText(copyString);
    });

    // Open in new split.
    if (hoveredElement->getLink().type == Link::Url)
    {
        static QRegularExpression twitchChannelRegex(
            R"(^(?:https?:\/\/)?(?:www\.|go\.)?twitch\.tv\/(?<username>[a-z0-9_]{3,}))",
            QRegularExpression::CaseInsensitiveOption);
        static QSet<QString> ignoredUsernames{
            "videos",    "settings", "directory",     "jobs",     "friends",
            "inventory", "payments", "subscriptions", "messages",
        };

        auto twitchMatch =
            twitchChannelRegex.match(hoveredElement->getLink().value);
        auto twitchUsername = twitchMatch.captured("username");
        if (!twitchUsername.isEmpty() &&
            !ignoredUsernames.contains(twitchUsername))
        {
            menu->addSeparator();
            menu->addAction("Open in new split", [twitchUsername, this] {
                this->joinToChannel.invoke(twitchUsername);
            });
        }
    }

    menu->popup(QCursor::pos());
    menu->raise();

    return;
}

void ChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex))
    {
        return;
    }

    // message under cursor is collapsed
    if (layout->flags.has(MessageLayoutFlag::Collapsed))
    {
        return;
    }

    const MessageLayoutElement *hoverLayoutElement =
        layout->getElementAt(relativePos);
    this->lastDClickPosition_ = event->screenPos();

    if (hoverLayoutElement == nullptr)
    {
        // Possibility for triple click which doesn't have to be over an
        // existing layout element
        this->clickTimer_->start();
        return;
    }
    if (!this->isMouseDown_)
    {
        this->isDoubleClick_ = true;

        int wordStart;
        int wordEnd;
        this->getWordBounds(layout.get(), hoverLayoutElement, relativePos,
                            wordStart, wordEnd);

        this->clickTimer_->start();

        SelectionItem wordMin(messageIndex, wordStart);
        SelectionItem wordMax(messageIndex, wordEnd);

        this->dCSelection_.originalStart = wordStart;
        this->dCSelection_.originalEnd = wordEnd;
        this->dCSelection_.origMessageIndex = messageIndex;
        this->dCSelection_.origStartItem = wordMin;
        this->dCSelection_.origEndItem = wordMax;

        this->setSelection(wordMin, wordMax);
    }

    if (getSettings()->linksDoubleClickOnly)
    {
        auto &link = hoverLayoutElement->getLink();
        this->handleLinkClick(event, link, layout.get());
    }
}

void ChannelView::hideEvent(QHideEvent *)
{
    for (auto &layout : this->messagesOnScreen_)
    {
        layout->deleteBuffer();
    }

    this->messagesOnScreen_.clear();
}

void ChannelView::showUserInfoPopup(const QString &userName)
{
    auto *userPopup = new UserInfoPopup;
    userPopup->setData(userName, this->channel_);
    userPopup->setActionOnFocusLoss(BaseWindow::Delete);
    QPoint offset(int(150 * this->getScale()), int(70 * this->getScale()));
    userPopup->move(QCursor::pos() - offset);
    userPopup->show();
}

void ChannelView::handleLinkClick(QMouseEvent *event, const Link &link,
                                  MessageLayout *layout)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    switch (link.type)
    {
        case Link::UserWhisper:
        case Link::UserInfo:
        {
            auto user = link.value;
            this->showUserInfoPopup(user);
            qDebug() << "Clicked " << user << "s message";
        }
        break;

        case Link::Url:
        {
            QDesktopServices::openUrl(QUrl(link.value));
        }
        break;

        case Link::UserAction:
        {
            QString value = link.value;
            value.replace("{user}", layout->getMessage()->loginName);
            this->channel_->sendMessage(value);
        }
        break;

        default:;
    }
}

bool ChannelView::tryGetMessageAt(QPoint p,
                                  std::shared_ptr<MessageLayout> &_message,
                                  QPoint &relativePos, int &index)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = this->scrollBar_->getCurrentValue();

    if (start >= messagesSnapshot.size())
    {
        return false;
    }

    int y = -(messagesSnapshot[start]->getHeight() *
              (fmod(this->scrollBar_->getCurrentValue(), 1)));

    for (size_t i = start; i < messagesSnapshot.size(); ++i)
    {
        auto message = messagesSnapshot[i];

        if (p.y() < y + message->getHeight())
        {
            relativePos = QPoint(p.x(), p.y() - y);
            _message = message;
            index = i;
            return true;
        }

        y += message->getHeight();
    }

    return false;
}

int ChannelView::getLayoutWidth() const
{
    if (this->scrollBar_->isVisible())
        return int(this->width() - 8 * this->getScale());

    return this->width();
}

void ChannelView::selectWholeMessage(MessageLayout *layout, int &messageIndex)
{
    SelectionItem msgStart(messageIndex,
                           layout->getFirstMessageCharacterIndex());
    SelectionItem msgEnd(messageIndex, layout->getLastCharacterIndex());
    this->setSelection(msgStart, msgEnd);
}

void ChannelView::getWordBounds(MessageLayout *layout,
                                const MessageLayoutElement *element,
                                const QPoint &relativePos, int &wordStart,
                                int &wordEnd)
{
    const int mouseInWordIndex = element->getMouseOverIndex(relativePos);
    wordStart = layout->getSelectionIndex(relativePos) - mouseInWordIndex;
    const int selectionLength = element->getSelectionIndexCount();
    const int length =
        element->hasTrailingSpace() ? selectionLength - 1 : selectionLength;
    wordEnd = wordStart + length;
}

}  // namespace chatterino
