#include "ChannelView.hpp"

#include "Application.hpp"
#include "debug/Benchmark.hpp"
#include "debug/Log.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Message.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DistanceBetweenPoints.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/splits/Split.hpp"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsBlurEffect>
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

ChannelView::ChannelView(BaseWidget *parent)
    : BaseWidget(parent)
    , scrollBar_(this)
{
    auto app = getApp();

    this->setMouseTracking(true);

    this->connections_.push_back(app->windows->wordFlagsChanged.connect([this] {
        this->layoutMessages();
        this->update();
    }));

    this->scrollBar_.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->actuallyLayoutMessages(true);

        //        if (!this->isPaused()) {
        this->goToBottom_->setVisible(this->enableScrollingToBottom_ &&
                                      this->scrollBar_.isVisible() &&
                                      !this->scrollBar_.isAtBottom());
        //        }

        this->queueUpdate();
    });

    this->scrollBar_.getDesiredValueChanged().connect(
        [this] { this->pausedByScrollingUp_ = !this->scrollBar_.isAtBottom(); });

    this->connections_.push_back(app->windows->repaintGifs.connect([&] {
        this->queueUpdate();  //
    }));

    this->connections_.push_back(app->windows->layout.connect([&](Channel *channel) {
        if (channel == nullptr || this->channel_.get() == channel) {
            this->layoutMessages();
        }
    }));

    this->goToBottom_ = new RippleEffectLabel(this, 0);
    this->goToBottom_->setStyleSheet("background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom_->getLabel().setText("More messages below");
    this->goToBottom_->setVisible(false);

    this->connections_.emplace_back(app->fonts->fontChanged.connect([this] {
        this->layoutMessages();  //
    }));

    QObject::connect(this->goToBottom_, &RippleEffectLabel::clicked, this, [=] {
        QTimer::singleShot(180, [=] {
            this->scrollBar_.scrollToBottom(
                app->settings->enableSmoothScrollingNewMessages.getValue());
        });
    });

    //    this->updateTimer.setInterval(1000 / 60);
    //    this->updateTimer.setSingleShot(true);
    //    connect(&this->updateTimer, &QTimer::timeout, this, [this] {
    //        if (this->updateQueued) {
    //            this->updateQueued = false;
    //            this->repaint();
    //            this->updateTimer.start();
    //        }
    //    });

    this->pauseTimeout_.setSingleShot(true);
    QObject::connect(&this->pauseTimeout_, &QTimer::timeout, [this] {
        this->pausedTemporarily_ = false;
        this->updatePauseStatus();
        this->layoutMessages();
    });

    app->settings->showLastMessageIndicator.connect(
        [this](auto, auto) {
            this->update();  //
        },
        this->connections_);

    this->layoutCooldown_ = new QTimer(this);
    this->layoutCooldown_->setSingleShot(true);
    this->layoutCooldown_->setInterval(66);

    QObject::connect(this->layoutCooldown_, &QTimer::timeout, [this] {
        if (this->layoutQueued_) {
            this->layoutMessages();
            this->layoutQueued_ = false;
        }
    });

    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+C"), this);
    QObject::connect(shortcut, &QShortcut::activated,
                     [this] { QGuiApplication::clipboard()->setText(this->getSelectedText()); });
}

ChannelView::~ChannelView()
{
}

void ChannelView::themeChangedEvent()
{
    BaseWidget::themeChangedEvent();

    this->layoutMessages();
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

void ChannelView::layoutMessages()
{
    //    if (!this->layoutCooldown->isActive()) {
    this->actuallyLayoutMessages();

    //        this->layoutCooldown->start();
    //    } else {
    //        this->layoutQueued = true;
    //    }
}

void ChannelView::actuallyLayoutMessages(bool causedByScrollbar)
{
    //    BenchmarkGuard benchmark("layout messages");

    auto app = getApp();

    auto messagesSnapshot = this->getMessagesSnapshot();

    if (messagesSnapshot.getLength() == 0) {
        this->scrollBar_.setVisible(false);

        return;
    }

    bool redrawRequired = false;
    bool showScrollbar = false;

    // Bool indicating whether or not we were showing all messages
    // True if one of the following statements are true:
    // The scrollbar was not visible
    // The scrollbar was visible and at the bottom
    this->showingLatestMessages_ = this->scrollBar_.isAtBottom() || !this->scrollBar_.isVisible();

    size_t start = size_t(this->scrollBar_.getCurrentValue());
    int layoutWidth = this->getLayoutWidth();

    MessageElement::Flags flags = this->getFlags();

    // layout the visible messages in the view
    if (messagesSnapshot.getLength() > start) {
        int y = int(-(messagesSnapshot[start]->getHeight() *
                      (fmod(this->scrollBar_.getCurrentValue(), 1))));

        for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
            auto message = messagesSnapshot[i];

            redrawRequired |= message->layout(layoutWidth, this->getScale(), flags);

            y += message->getHeight();

            if (y >= this->height()) {
                break;
            }
        }
    }

    // layout the messages at the bottom to determine the scrollbar thumb size
    int h = this->height() - 8;

    for (int i = int(messagesSnapshot.getLength()) - 1; i >= 0; i--) {
        auto *message = messagesSnapshot[i].get();

        message->layout(layoutWidth, this->getScale(), flags);

        h -= message->getHeight();

        if (h < 0) {
            this->scrollBar_.setLargeChange((messagesSnapshot.getLength() - i) +
                                            qreal(h) / message->getHeight());
            //            this->scrollBar.setDesiredValue(this->scrollBar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    this->scrollBar_.setVisible(showScrollbar);

    if (!showScrollbar && !causedByScrollbar) {
        this->scrollBar_.setDesiredValue(0);
    }

    this->scrollBar_.setMaximum(messagesSnapshot.getLength());

    // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
    // to bottom
    if (this->enableScrollingToBottom_ && this->showingLatestMessages_ && showScrollbar) {
        if (!this->isPaused()) {
            this->scrollBar_.scrollToBottom(
                //                this->messageWasAdded &&
                app->settings->enableSmoothScrollingNewMessages.getValue());
        }
        this->messageWasAdded_ = false;
    }

    if (redrawRequired) {
        this->queueUpdate();
    }
}

void ChannelView::clearMessages()
{
    // Clear all stored messages in this chat widget
    this->messages.clear();

    // Layout chat widget messages, and force an update regardless if there are no messages
    this->layoutMessages();
    this->queueUpdate();
}

Scrollbar &ChannelView::getScrollBar()
{
    return this->scrollBar_;
}

QString ChannelView::getSelectedText()
{
    QString result = "";

    LimitedQueueSnapshot<MessageLayoutPtr> messagesSnapshot = this->getMessagesSnapshot();

    Selection _selection = this->selection_;

    if (_selection.isEmpty()) {
        return result;
    }

    for (int msg = _selection.selectionMin.messageIndex;
         msg <= _selection.selectionMax.messageIndex; msg++) {
        MessageLayoutPtr layout = messagesSnapshot[msg];
        int from =
            msg == _selection.selectionMin.messageIndex ? _selection.selectionMin.charIndex : 0;
        int to = msg == _selection.selectionMax.messageIndex ? _selection.selectionMax.charIndex
                                                             : layout->getLastCharacterIndex() + 1;

        qDebug() << "from:" << from << ", to:" << to;

        layout->addSelectionText(result, from, to);
    }
    qDebug() << "xd <";

    return result;
}

bool ChannelView::hasSelection()
{
    return !this->selection_.isEmpty();
}

void ChannelView::clearSelection()
{
    this->selection_ = Selection();
    layoutMessages();
}

void ChannelView::setEnableScrollingToBottom(bool value)
{
    this->enableScrollingToBottom_ = value;
}

bool ChannelView::getEnableScrollingToBottom() const
{
    return this->enableScrollingToBottom_;
}

void ChannelView::setOverrideFlags(boost::optional<MessageElement::Flags> value)
{
    this->overrideFlags_ = value;
}

const boost::optional<MessageElement::Flags> &ChannelView::getOverrideFlags() const
{
    return this->overrideFlags_;
}

LimitedQueueSnapshot<MessageLayoutPtr> ChannelView::getMessagesSnapshot()
{
    if (!this->isPaused() /*|| this->scrollBar_.isVisible()*/) {
        this->snapshot_ = this->messages.getSnapshot();
    }

    return this->snapshot_;
}

void ChannelView::setChannel(ChannelPtr newChannel)
{
    if (this->channel_) {
        this->detachChannel();
    }

    this->messages.clear();

    // on new message
    this->channelConnections_.push_back(
        newChannel->messageAppended.connect([this](MessagePtr &message) {
            MessageLayoutPtr deleted;

            auto messageRef = new MessageLayout(message);

            if (this->lastMessageHasAlternateBackground_) {
                messageRef->flags |= MessageLayout::AlternateBackground;
            }
            this->lastMessageHasAlternateBackground_ = !this->lastMessageHasAlternateBackground_;

            if (this->isPaused()) {
                this->messagesAddedSinceSelectionPause_++;
            }

            if (this->messages.pushBack(MessageLayoutPtr(messageRef), deleted)) {
                //                if (!this->isPaused()) {
                if (this->scrollBar_.isAtBottom()) {
                    this->scrollBar_.scrollToBottom();
                } else {
                    this->scrollBar_.offset(-1);
                }
                //                }
            }

            if (!(message->flags & Message::DoNotTriggerNotification)) {
                if (message->flags & Message::Highlighted) {
                    this->tabHighlightRequested.invoke(HighlightState::Highlighted);
                } else {
                    this->tabHighlightRequested.invoke(HighlightState::NewMessage);
                }
            }

            this->scrollBar_.addHighlight(message->getScrollBarHighlight());

            this->messageWasAdded_ = true;
            this->layoutMessages();
        }));

    this->channelConnections_.push_back(
        newChannel->messagesAddedAtStart.connect([this](std::vector<MessagePtr> &messages) {
            std::vector<MessageLayoutPtr> messageRefs;
            messageRefs.resize(messages.size());
            for (size_t i = 0; i < messages.size(); i++) {
                messageRefs.at(i) = MessageLayoutPtr(new MessageLayout(messages.at(i)));
            }

            if (!this->isPaused()) {
                if (this->messages.pushFront(messageRefs).size() > 0) {
                    if (this->scrollBar_.isAtBottom()) {
                        this->scrollBar_.scrollToBottom();
                    } else {
                        this->scrollBar_.offset(qreal(messages.size()));
                    }
                }
            }

            std::vector<ScrollbarHighlight> highlights;
            highlights.reserve(messages.size());
            for (size_t i = 0; i < messages.size(); i++) {
                highlights.push_back(messages.at(i)->getScrollBarHighlight());
            }

            this->scrollBar_.addHighlightsAtStart(highlights);

            this->messageWasAdded_ = true;
            this->layoutMessages();
        }));

    // on message removed
    this->channelConnections_.push_back(
        newChannel->messageRemovedFromStart.connect([this](MessagePtr &) {
            this->selection_.selectionMin.messageIndex--;
            this->selection_.selectionMax.messageIndex--;
            this->selection_.start.messageIndex--;
            this->selection_.end.messageIndex--;

            this->layoutMessages();
        }));

    // on message replaced
    this->channelConnections_.push_back(
        newChannel->messageReplaced.connect([this](size_t index, MessagePtr replacement) {
            if (index >= this->messages.getSnapshot().getLength() || index < 0) {
                return;
            }

            MessageLayoutPtr newItem(new MessageLayout(replacement));
            auto snapshot = this->messages.getSnapshot();
            if (index >= snapshot.getLength()) {
                Log("Tried to replace out of bounds message. Index: {}. Length: {}", index,
                    snapshot.getLength());
                return;
            }

            const auto &message = snapshot[index];
            if (message->flags & MessageLayout::AlternateBackground) {
                newItem->flags |= MessageLayout::AlternateBackground;
            }

            this->scrollBar_.replaceHighlight(index, replacement->getScrollBarHighlight());

            this->messages.replaceItem(message, newItem);
            this->layoutMessages();
        }));

    auto snapshot = newChannel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        MessageLayoutPtr deleted;

        auto messageRef = new MessageLayout(snapshot[i]);

        if (this->lastMessageHasAlternateBackground_) {
            messageRef->flags |= MessageLayout::AlternateBackground;
        }
        this->lastMessageHasAlternateBackground_ = !this->lastMessageHasAlternateBackground_;

        this->messages.pushBack(MessageLayoutPtr(messageRef), deleted);
    }

    this->channel_ = newChannel;

    this->layoutMessages();
    this->queueUpdate();
}

void ChannelView::detachChannel()
{
    this->channelConnections_.clear();
}

void ChannelView::pause(int msecTimeout)
{
    this->pausedTemporarily_ = true;
    this->updatePauseStatus();

    if (this->pauseTimeout_.remainingTime() < msecTimeout) {
        this->pauseTimeout_.stop();
        this->pauseTimeout_.start(msecTimeout);

        //        qDebug() << "pause" << msecTimeout;
    }
}

void ChannelView::updateLastReadMessage()
{
    auto _snapshot = this->getMessagesSnapshot();

    if (_snapshot.getLength() > 0) {
        this->lastReadMessage_ = _snapshot[_snapshot.getLength() - 1];
    }

    this->update();
}

void ChannelView::resizeEvent(QResizeEvent *)
{
    this->scrollBar_.setGeometry(this->width() - this->scrollBar_.width(), 0,
                                 this->scrollBar_.width(), this->height());

    this->goToBottom_->setGeometry(0, this->height() - 32, this->width(), 32);

    this->scrollBar_.raise();

    this->layoutMessages();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start, const SelectionItem &end)
{
    // selections
    if (!this->selecting_ && start != end) {
        this->messagesAddedSinceSelectionPause_ = 0;

        this->selecting_ = true;
        this->pausedBySelection_ = true;
    }

    this->selection_ = Selection(start, end);

    this->selectionChanged.invoke();
}

MessageElement::Flags ChannelView::getFlags() const
{
    auto app = getApp();

    if (this->overrideFlags_) {
        return this->overrideFlags_.get();
    }

    MessageElement::Flags flags = app->windows->getWordFlags();

    Split *split = dynamic_cast<Split *>(this->parentWidget());

    if (split != nullptr) {
        if (split->getModerationMode()) {
            flags = MessageElement::Flags(flags | MessageElement::ModeratorTools);
        }
        if (this->channel_ == app->twitch.server->mentionsChannel) {
            flags = MessageElement::Flags(flags | MessageElement::ChannelName);
        }
    }

    return flags;
}

bool ChannelView::isPaused()
{
    return false;
    //    return this->pausedTemporarily_ || this->pausedBySelection_ || this->pausedByScrollingUp_;
}

void ChannelView::updatePauseStatus()
{
    if (this->isPaused()) {
        this->scrollBar_.pauseHighlights();
    } else {
        this->scrollBar_.unpauseHighlights();
    }
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BenchmarkGuard benchmark("paint event");

    QPainter painter(this);

    painter.fillRect(rect(), this->theme->splits.background);

    // draw messages
    this->drawMessages(painter);
}

// if overlays is false then it draws the message, if true then it draws things such as the grey
// overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter)
{
    auto app = getApp();

    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = size_t(this->scrollBar_.getCurrentValue());

    if (start >= messagesSnapshot.getLength()) {
        return;
    }

    int y = int(-(messagesSnapshot[start].get()->getHeight() *
                  (fmod(this->scrollBar_.getCurrentValue(), 1))));

    MessageLayout *end = nullptr;
    bool windowFocused = this->window() == QApplication::activeWindow();

    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        MessageLayout *layout = messagesSnapshot[i].get();

        bool isLastMessage = false;
        if (app->settings->showLastMessageIndicator) {
            isLastMessage = this->lastReadMessage_.get() == layout;
        }

        layout->paint(painter, DRAW_WIDTH, y, i, this->selection_, isLastMessage, windowFocused);

        y += layout->getHeight();

        end = layout;
        if (y > this->height()) {
            break;
        }
    }

    if (end == nullptr) {
        return;
    }

    // remove messages that are on screen
    // the messages that are left at the end get their buffers reset
    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        auto it = this->messagesOnScreen_.find(messagesSnapshot[i]);
        if (it != this->messagesOnScreen_.end()) {
            this->messagesOnScreen_.erase(it);
        }
    }

    // delete the message buffers that aren't on screen
    for (const std::shared_ptr<MessageLayout> &item : this->messagesOnScreen_) {
        item->deleteBuffer();
    }

    this->messagesOnScreen_.clear();

    // add all messages on screen to the map
    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        std::shared_ptr<MessageLayout> layout = messagesSnapshot[i];

        this->messagesOnScreen_.insert(layout);

        if (layout.get() == end) {
            break;
        }
    }
}

void ChannelView::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() != Qt::Vertical) {
        return;
    }

    this->pausedBySelection_ = false;
    this->pausedTemporarily_ = false;
    this->updatePauseStatus();

    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }

    if (this->scrollBar_.isVisible()) {
        auto app = getApp();

        float mouseMultiplier = app->settings->mouseScrollMultiplier;

        qreal desired = this->scrollBar_.getDesiredValue();
        qreal delta = event->delta() * qreal(1.5) * mouseMultiplier;

        auto snapshot = this->getMessagesSnapshot();
        int snapshotLength = int(snapshot.getLength());
        int i = std::min<int>(int(desired), snapshotLength);

        if (delta > 0) {
            qreal scrollFactor = fmod(desired, 1);
            qreal currentScrollLeft = int(scrollFactor * snapshot[i]->getHeight());

            for (; i >= 0; i--) {
                if (delta < currentScrollLeft) {
                    desired -= scrollFactor * (delta / currentScrollLeft);
                    break;
                } else {
                    delta -= currentScrollLeft;
                    desired -= scrollFactor;
                }

                if (i == 0) {
                    desired = 0;
                } else {
                    snapshot[i - 1]->layout(this->getLayoutWidth(), this->getScale(),
                                            this->getFlags());
                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i - 1]->getHeight();
                }
            }
        } else {
            delta = -delta;
            qreal scrollFactor = 1 - fmod(desired, 1);
            qreal currentScrollLeft = int(scrollFactor * snapshot[i]->getHeight());

            for (; i < snapshotLength; i++) {
                if (delta < currentScrollLeft) {
                    desired += scrollFactor * (qreal(delta) / currentScrollLeft);
                    break;
                } else {
                    delta -= currentScrollLeft;
                    desired += scrollFactor;
                }

                if (i == snapshotLength - 1) {
                    desired = snapshot.getLength();
                } else {
                    snapshot[i + 1]->layout(this->getLayoutWidth(), this->getScale(),
                                            this->getFlags());

                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i + 1]->getHeight();
                }
            }
        }

        this->scrollBar_.setDesiredValue(desired, true);
    }
}

void ChannelView::enterEvent(QEvent *)
{
    //    this->pause(PAUSE_TIME);
}

void ChannelView::leaveEvent(QEvent *)
{
    this->pausedTemporarily_ = false;
    this->updatePauseStatus();
    this->layoutMessages();
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier)) {
        this->unsetCursor();

        event->ignore();
        return;
    }

    auto app = getApp();

    if (app->settings->pauseChatHover.getValue()) {
        this->pause(CHAT_HOVER_PAUSE_DURATION);
    }

    auto tooltipWidget = TooltipWidget::getInstance();
    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    // is selecting
    if (this->isMouseDown_) {
        this->pause(300);
        int index = layout->getSelectionIndex(relativePos);

        this->setSelection(this->selection_.start, SelectionItem(messageIndex, index));

        this->queueUpdate();
    }

    // message under cursor is collapsed
    if (layout->flags & MessageLayout::Collapsed) {
        this->setCursor(Qt::PointingHandCursor);
        tooltipWidget->hide();
        return;
    }

    // check if word underneath cursor
    const MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr) {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }
    const auto &tooltip = hoverLayoutElement->getCreator().getTooltip();

    if (tooltip.isEmpty()) {
        tooltipWidget->hide();
    } else {
        tooltipWidget->moveTo(this, event->globalPos());
        tooltipWidget->setText(tooltip);
        tooltipWidget->adjustSize();
        tooltipWidget->show();
        tooltipWidget->raise();
    }

    // check if word has a link
    if (hoverLayoutElement->getLink().isValid()) {
        this->setCursor(Qt::PointingHandCursor);
    } else {
        this->setCursor(Qt::ArrowCursor);
    }
}

void ChannelView::mousePressEvent(QMouseEvent *event)
{
    auto app = getApp();

    this->mouseDown.invoke(event);

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        setCursor(Qt::ArrowCursor);

        auto messagesSnapshot = this->getMessagesSnapshot();
        if (messagesSnapshot.getLength() == 0) {
            return;
        }

        // Start selection at the last message at its last index
        if (event->button() == Qt::LeftButton) {
            auto lastMessageIndex = messagesSnapshot.getLength() - 1;
            auto lastMessage = messagesSnapshot[lastMessageIndex];
            auto lastCharacterIndex = lastMessage->getLastCharacterIndex();

            SelectionItem selectionItem(lastMessageIndex, lastCharacterIndex);
            this->setSelection(selectionItem, selectionItem);
        }
        return;
    }

    // check if message is collapsed
    switch (event->button()) {
        case Qt::LeftButton: {
            this->lastPressPosition_ = event->screenPos();
            this->isMouseDown_ = true;

            if (layout->flags & MessageLayout::Collapsed) {
                return;
            }

            if (app->settings->linksDoubleClickOnly.getValue()) {
                this->pause(200);
            }

            int index = layout->getSelectionIndex(relativePos);

            auto selectionItem = SelectionItem(messageIndex, index);
            this->setSelection(selectionItem, selectionItem);
        } break;

        case Qt::RightButton: {
            this->lastRightPressPosition_ = event->screenPos();
            this->isRightMouseDown_ = true;
        } break;

        default:;
    }

    this->update();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    // check if mouse was pressed
    if (event->button() == Qt::LeftButton) {
        if (this->isMouseDown_) {
            this->isMouseDown_ = false;

            if (fabsf(distanceBetweenPoints(this->lastPressPosition_, event->screenPos())) > 15.f) {
                return;
            }
        } else {
            return;
        }
    } else if (event->button() == Qt::RightButton) {
        if (this->isRightMouseDown_) {
            this->isRightMouseDown_ = false;

            if (fabsf(distanceBetweenPoints(this->lastRightPressPosition_, event->screenPos())) >
                15.f) {
                return;
            }
        } else {
            return;
        }
    } else {
        // not left or right button
        return;
    }

    // find message
    this->layoutMessages();

    std::shared_ptr<MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message found
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        // No message at clicked position
        return;
    }

    // message under cursor is collapsed
    if (layout->flags & MessageLayout::Collapsed) {
        layout->flags |= MessageLayout::Expanded;
        layout->flags |= MessageLayout::RequiresLayout;

        this->layoutMessages();
        return;
    }

    const MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr) {
        return;
    }

    // handle the click
    this->handleMouseClick(event, hoverLayoutElement, layout.get());
}

void ChannelView::handleMouseClick(QMouseEvent *event, const MessageLayoutElement *hoveredElement,
                                   MessageLayout *layout)
{
    switch (event->button()) {
        case Qt::LeftButton: {
            if (this->selecting_) {
                if (this->messagesAddedSinceSelectionPause_ >
                    SELECTION_RESUME_SCROLLING_MSG_THRESHOLD) {
                    this->showingLatestMessages_ = false;
                }

                // this->pausedBySelection = false;
                this->selecting_ = false;
                // this->pauseTimeout.stop();
                // this->pausedTemporarily = false;

                this->layoutMessages();
            }

            auto &link = hoveredElement->getLink();
            if (!getApp()->settings->linksDoubleClickOnly) {
                this->handleLinkClick(event, link, layout);

                this->linkClicked.invoke(link);
            }
        } break;
        case Qt::RightButton: {
            auto &link = hoveredElement->getLink();
            if (link.type == Link::UserInfo) {
                Split *split = dynamic_cast<Split *>(this->parentWidget());
                if (split != nullptr) {
                    split->insertTextToInput("@" + link.value + ", ");
                }
            } else {
                this->addContextMenuItems(hoveredElement, layout);
            }
        } break;
        default:;
    }
}

void ChannelView::addContextMenuItems(const MessageLayoutElement *hoveredElement,
                                      MessageLayout *layout)
{
    const auto &creator = hoveredElement->getCreator();
    auto creatorFlags = creator.getFlags();

    static QMenu *menu = new QMenu;
    menu->clear();

    // Emote actions
    if (creatorFlags & (MessageElement::Flags::EmoteImages | MessageElement::Flags::EmojiImage)) {
        const auto &emoteElement = static_cast<const EmoteElement &>(creator);

        // TODO: We might want to add direct "Open image" variants alongside the Copy
        // actions
        if (emoteElement.data.image1x != nullptr) {
            QAction *addEntry = menu->addAction("Copy emote link...");

            QMenu *procmenu = new QMenu;
            addEntry->setMenu(procmenu);
            procmenu->addAction("Copy 1x link", [url = emoteElement.data.image1x->getUrl()] {
                QApplication::clipboard()->setText(url);  //
            });
            if (emoteElement.data.image2x != nullptr) {
                procmenu->addAction("Copy 2x link", [url = emoteElement.data.image2x->getUrl()] {
                    QApplication::clipboard()->setText(url);  //
                });
            }
            if (emoteElement.data.image3x != nullptr) {
                procmenu->addAction("Copy 3x link", [url = emoteElement.data.image3x->getUrl()] {
                    QApplication::clipboard()->setText(url);  //
                });
            }
            if ((creatorFlags & MessageElement::Flags::BttvEmote) != 0) {
                procmenu->addSeparator();
                QString emotePageLink = emoteElement.data.pageLink;
                procmenu->addAction("Copy BTTV emote link", [emotePageLink] {
                    QApplication::clipboard()->setText(emotePageLink);  //
                });
            } else if ((creatorFlags & MessageElement::Flags::FfzEmote) != 0) {
                procmenu->addSeparator();
                QString emotePageLink = emoteElement.data.pageLink;
                procmenu->addAction("Copy FFZ emote link", [emotePageLink] {
                    QApplication::clipboard()->setText(emotePageLink);  //
                });
            }
        }
        if (emoteElement.data.image1x != nullptr) {
            QAction *addEntry = menu->addAction("Open emote link...");

            QMenu *procmenu = new QMenu;
            addEntry->setMenu(procmenu);
            procmenu->addAction("Open 1x link", [url = emoteElement.data.image1x->getUrl()] {
                QDesktopServices::openUrl(QUrl(url));  //
            });
            if (emoteElement.data.image2x != nullptr) {
                procmenu->addAction("Open 2x link", [url = emoteElement.data.image2x->getUrl()] {
                    QDesktopServices::openUrl(QUrl(url));  //
                });
            }
            if (emoteElement.data.image3x != nullptr) {
                procmenu->addAction("Open 3x link", [url = emoteElement.data.image3x->getUrl()] {
                    QDesktopServices::openUrl(QUrl(url));  //
                });
            }
            if ((creatorFlags & MessageElement::Flags::BttvEmote) != 0) {
                procmenu->addSeparator();
                QString emotePageLink = emoteElement.data.pageLink;
                procmenu->addAction("Open BTTV emote link", [emotePageLink] {
                    QDesktopServices::openUrl(QUrl(emotePageLink));  //
                });
            } else if ((creatorFlags & MessageElement::Flags::FfzEmote) != 0) {
                procmenu->addSeparator();
                QString emotePageLink = emoteElement.data.pageLink;
                procmenu->addAction("Open FFZ emote link", [emotePageLink] {
                    QDesktopServices::openUrl(QUrl(emotePageLink));  //
                });
            }
        }
    }

    // add seperator
    if (!menu->actions().empty()) {
        menu->addSeparator();
    }

    // Link copy
    if (hoveredElement->getLink().type == Link::Url) {
        QString url = hoveredElement->getLink().value;

        menu->addAction("Open link", [url] { QDesktopServices::openUrl(QUrl(url)); });
        menu->addAction("Copy link", [url] { QApplication::clipboard()->setText(url); });

        menu->addSeparator();
    }

    // Copy actions
    if (!this->selection_.isEmpty()) {
        menu->addAction("Copy selection",
                        [this] { QGuiApplication::clipboard()->setText(this->getSelectedText()); });
    }

    menu->addAction("Copy message", [layout] {
        QString copyString;
        layout->addSelectionText(copyString);

        QGuiApplication::clipboard()->setText(copyString);
    });

    //        menu->addAction("Quote message", [layout] {
    //            QString copyString;
    //            layout->addSelectionText(copyString);

    //            // insert into input
    //        });

    menu->popup(QCursor::pos());
    menu->raise();

    return;
}

void ChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
    auto app = getApp();

    if (app->settings->linksDoubleClickOnly) {
        std::shared_ptr<MessageLayout> layout;
        QPoint relativePos;
        int messageIndex;

        if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
            return;
        }

        // message under cursor is collapsed
        if (layout->flags & MessageLayout::Collapsed) {
            return;
        }

        const MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

        if (hoverLayoutElement == nullptr) {
            return;
        }

        auto &link = hoverLayoutElement->getLink();
        this->handleLinkClick(event, link, layout.get());
    }
}

void ChannelView::hideEvent(QHideEvent *)
{
    for (auto &layout : this->messagesOnScreen_) {
        layout->deleteBuffer();
    }

    this->messagesOnScreen_.clear();
}

void ChannelView::handleLinkClick(QMouseEvent *event, const Link &link, MessageLayout *layout)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    switch (link.type) {
        case Link::UserInfo: {
            auto user = link.value;

            auto *userPopup = new UserInfoPopup;
            userPopup->setData(user, this->channel_);
            userPopup->setActionOnFocusLoss(BaseWindow::Delete);
            QPoint offset(int(150 * this->getScale()), int(70 * this->getScale()));
            userPopup->move(QCursor::pos() - offset);
            userPopup->show();

            qDebug() << "Clicked " << user << "s message";
            break;
        }

        case Link::Url: {
            QDesktopServices::openUrl(QUrl(link.value));
            break;
        }

        case Link::UserAction: {
            QString value = link.value;
            value.replace("{user}", layout->getMessage()->loginName);
            this->channel_->sendMessage(value);
        }

        default:;
    }
}

bool ChannelView::tryGetMessageAt(QPoint p, std::shared_ptr<MessageLayout> &_message,
                                  QPoint &relativePos, int &index)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = this->scrollBar_.getCurrentValue();

    if (start >= messagesSnapshot.getLength()) {
        return false;
    }

    int y = -(messagesSnapshot[start]->getHeight() * (fmod(this->scrollBar_.getCurrentValue(), 1)));

    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        auto message = messagesSnapshot[i];

        if (p.y() < y + message->getHeight()) {
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
    if (this->scrollBar_.isVisible())
        return int(this->width() - 8 * this->getScale());

    return this->width();
}

}  // namespace chatterino
