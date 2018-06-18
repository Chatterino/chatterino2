#include "channelview.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/benchmark.hpp"
#include "util/distancebetweenpoints.hpp"
#include "widgets/split.hpp"
#include "widgets/tooltipwidget.hpp"
#include "widgets/userinfopopup.hpp"

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
#define CHAT_HOVER_PAUSE_DURATION 400

using namespace chatterino::messages;
using namespace chatterino::providers::twitch;

namespace chatterino {
namespace widgets {

ChannelView::ChannelView(BaseWidget *parent)
    : BaseWidget(parent)
    , scrollBar(this)
{
    auto app = getApp();

    this->setMouseTracking(true);

    this->connections_.push_back(app->settings->wordFlagsChanged.connect([this] {
        this->layoutMessages();
        this->update();
    }));

    this->scrollBar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->actuallyLayoutMessages(true);

        if (!this->isPaused()) {
            this->goToBottom->setVisible(this->enableScrollingToBottom &&
                                         this->scrollBar.isVisible() &&
                                         !this->scrollBar.isAtBottom());
        }

        this->queueUpdate();
    });

    this->connections_.push_back(app->windows->repaintGifs.connect([&] {
        this->queueUpdate();  //
    }));

    this->connections_.push_back(app->windows->layout.connect([&](Channel *channel) {
        if (channel == nullptr || this->channel.get() == channel) {
            this->layoutMessages();
        }
    }));

    this->goToBottom = new RippleEffectLabel(this, 0);
    this->goToBottom->setStyleSheet("background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom->getLabel().setText("More messages below");
    this->goToBottom->setVisible(false);

    this->connections_.emplace_back(app->fonts->fontChanged.connect([this] {
        this->layoutMessages();  //
    }));

    QObject::connect(goToBottom, &RippleEffectLabel::clicked, this, [=] {
        QTimer::singleShot(180, [=] {
            this->scrollBar.scrollToBottom(
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

    this->pauseTimeout.setSingleShot(true);
    QObject::connect(&this->pauseTimeout, &QTimer::timeout, [this] {
        this->pausedTemporarily = false;
        this->layoutMessages();
    });

    app->settings->showLastMessageIndicator.connect(
        [this](auto, auto) {
            this->update();  //
        },
        this->connections_);

    this->layoutCooldown = new QTimer(this);
    this->layoutCooldown->setSingleShot(true);
    this->layoutCooldown->setInterval(66);

    QObject::connect(this->layoutCooldown, &QTimer::timeout, [this] {
        if (this->layoutQueued) {
            this->layoutMessages();
            this->layoutQueued = false;
        }
    });

    QTimer::singleShot(1000, this, [this] {
        this->scrollBar.setGeometry(this->width() - this->scrollBar.width(), 0,
                                    this->scrollBar.width(), this->height());
    });

    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+C"), this);
    QObject::connect(shortcut, &QShortcut::activated,
                     [this] { QGuiApplication::clipboard()->setText(this->getSelectedText()); });
}

ChannelView::~ChannelView()
{
}

void ChannelView::themeRefreshEvent()
{
    BaseWidget::themeRefreshEvent();

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
        this->scrollBar.setVisible(false);

        return;
    }

    bool redrawRequired = false;
    bool showScrollbar = false;

    // Bool indicating whether or not we were showing all messages
    // True if one of the following statements are true:
    // The scrollbar was not visible
    // The scrollbar was visible and at the bottom
    this->showingLatestMessages = this->scrollBar.isAtBottom() || !this->scrollBar.isVisible();

    size_t start = this->scrollBar.getCurrentValue();
    //    int layoutWidth =
    //        (this->scrollBar.isVisible() ? width() - this->scrollBar.width() : width()) - 4;
    int layoutWidth = this->getLayoutWidth();

    MessageElement::Flags flags = this->getFlags();

    // layout the visible messages in the view
    if (messagesSnapshot.getLength() > start) {
        int y =
            -(messagesSnapshot[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

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

    for (int i = (int)messagesSnapshot.getLength() - 1; i >= 0; i--) {
        auto *message = messagesSnapshot[i].get();

        message->layout(layoutWidth, this->getScale(), flags);

        h -= message->getHeight();

        if (h < 0) {
            this->scrollBar.setLargeChange((messagesSnapshot.getLength() - i) +
                                           (qreal)h / message->getHeight());
            //            this->scrollBar.setDesiredValue(this->scrollBar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    this->scrollBar.setVisible(showScrollbar);

    if (!showScrollbar) {
        if (!causedByScrollbar) {
            this->scrollBar.setDesiredValue(0);
        }
    }

    this->scrollBar.setMaximum(messagesSnapshot.getLength());

    // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
    // to bottom
    // TODO: Do we want to check if the user is currently moving the scrollbar?
    // Perhaps also if the user scrolled with the scrollwheel in this ChatWidget in the last 0.2
    // seconds or something
    if (this->enableScrollingToBottom && this->showingLatestMessages && showScrollbar) {
        if (!this->isPaused()) {
            this->scrollBar.scrollToBottom(
                //                this->messageWasAdded &&
                app->settings->enableSmoothScrollingNewMessages.getValue());
        }
        this->messageWasAdded = false;
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
    return this->scrollBar;
}

QString ChannelView::getSelectedText()
{
    QString result = "";

    messages::LimitedQueueSnapshot<MessageLayoutPtr> messagesSnapshot = this->getMessagesSnapshot();

    Selection _selection = this->selection;

    if (_selection.isEmpty()) {
        return result;
    }

    qDebug() << "xd >";
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
    return !this->selection.isEmpty();
}

void ChannelView::clearSelection()
{
    this->selection = Selection();
    layoutMessages();
}

void ChannelView::setEnableScrollingToBottom(bool value)
{
    this->enableScrollingToBottom = value;
}

bool ChannelView::getEnableScrollingToBottom() const
{
    return this->enableScrollingToBottom;
}

void ChannelView::setOverrideFlags(boost::optional<messages::MessageElement::Flags> value)
{
    this->overrideFlags = value;
}

const boost::optional<messages::MessageElement::Flags> &ChannelView::getOverrideFlags() const
{
    return this->overrideFlags;
}

messages::LimitedQueueSnapshot<MessageLayoutPtr> ChannelView::getMessagesSnapshot()
{
    //    if (!this->isPaused()) {
    this->snapshot = this->messages.getSnapshot();
    //    }

    //    return this->snapshot;
    return this->snapshot;
}

void ChannelView::setChannel(ChannelPtr newChannel)
{
    if (this->channel) {
        this->detachChannel();
    }

    this->messages.clear();

    // on new message
    this->channelConnections_.push_back(
        newChannel->messageAppended.connect([this](MessagePtr &message) {
            MessageLayoutPtr deleted;

            auto messageRef = new MessageLayout(message);

            if (this->lastMessageHasAlternateBackground) {
                messageRef->flags |= MessageLayout::AlternateBackground;
            }
            this->lastMessageHasAlternateBackground = !this->lastMessageHasAlternateBackground;

            if (this->isPaused()) {
                this->messagesAddedSinceSelectionPause++;
            }

            if (this->messages.pushBack(MessageLayoutPtr(messageRef), deleted)) {
                //                if (!this->isPaused()) {
                if (this->scrollBar.isAtBottom()) {
                    this->scrollBar.scrollToBottom();
                } else {
                    this->scrollBar.offset(-1);
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

            this->scrollBar.addHighlight(message->getScrollBarHighlight());

            this->messageWasAdded = true;
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
                    if (this->scrollBar.isAtBottom()) {
                        this->scrollBar.scrollToBottom();
                    } else {
                        this->scrollBar.offset((qreal)messages.size());
                    }
                }
            }

            std::vector<ScrollbarHighlight> highlights;
            highlights.reserve(messages.size());
            for (size_t i = 0; i < messages.size(); i++) {
                highlights.push_back(messages.at(i)->getScrollBarHighlight());
            }

            this->scrollBar.addHighlightsAtStart(highlights);

            this->messageWasAdded = true;
            this->layoutMessages();
        }));

    // on message removed
    this->channelConnections_.push_back(
        newChannel->messageRemovedFromStart.connect([this](MessagePtr &) {
            this->selection.selectionMin.messageIndex--;
            this->selection.selectionMax.messageIndex--;
            this->selection.start.messageIndex--;
            this->selection.end.messageIndex--;

            this->layoutMessages();
        }));

    // on message replaced
    this->channelConnections_.push_back(
        newChannel->messageReplaced.connect([this](size_t index, MessagePtr replacement) {
            MessageLayoutPtr newItem(new MessageLayout(replacement));
            auto snapshot = this->messages.getSnapshot();
            if (index >= snapshot.getLength()) {
                debug::Log("Tried to replace out of bounds message. Index: {}. Length: {}", index,
                           snapshot.getLength());
                return;
            }

            const auto &message = snapshot[index];
            if (message->flags & MessageLayout::AlternateBackground) {
                newItem->flags |= MessageLayout::AlternateBackground;
            }

            this->scrollBar.replaceHighlight(index, replacement->getScrollBarHighlight());

            this->messages.replaceItem(message, newItem);
            this->layoutMessages();
        }));

    auto snapshot = newChannel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        MessageLayoutPtr deleted;

        auto messageRef = new MessageLayout(snapshot[i]);

        if (this->lastMessageHasAlternateBackground) {
            messageRef->flags |= MessageLayout::AlternateBackground;
        }
        this->lastMessageHasAlternateBackground = !this->lastMessageHasAlternateBackground;

        this->messages.pushBack(MessageLayoutPtr(messageRef), deleted);
    }

    this->channel = newChannel;

    this->layoutMessages();
    this->queueUpdate();
}

void ChannelView::detachChannel()
{
    this->channelConnections_.clear();
}

void ChannelView::pause(int msecTimeout)
{
    this->pausedTemporarily = true;

    this->pauseTimeout.start(msecTimeout);
}

void ChannelView::updateLastReadMessage()
{
    auto _snapshot = this->getMessagesSnapshot();

    if (_snapshot.getLength() > 0) {
        this->lastReadMessage = _snapshot[_snapshot.getLength() - 1];
    }

    this->update();
}

void ChannelView::resizeEvent(QResizeEvent *)
{
    this->scrollBar.setGeometry(this->width() - this->scrollBar.width(), 0, this->scrollBar.width(),
                                this->height());

    this->goToBottom->setGeometry(0, this->height() - 32, this->width(), 32);

    this->scrollBar.raise();

    this->layoutMessages();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start, const SelectionItem &end)
{
    // selections
    if (!this->selecting && start != end) {
        this->messagesAddedSinceSelectionPause = 0;

        this->selecting = true;
        this->pausedBySelection = true;
    }

    this->selection = Selection(start, end);

    this->selectionChanged.invoke();
}

messages::MessageElement::Flags ChannelView::getFlags() const
{
    auto app = getApp();

    if (this->overrideFlags) {
        return this->overrideFlags.get();
    }

    MessageElement::Flags flags = app->settings->getWordFlags();

    Split *split = dynamic_cast<Split *>(this->parentWidget());

    if (split != nullptr) {
        if (split->getModerationMode()) {
            flags = (MessageElement::Flags)(flags | MessageElement::ModeratorTools);
        }
        if (this->channel == app->twitch.server->mentionsChannel) {
            flags = (MessageElement::Flags)(flags | MessageElement::ChannelName);
        }
    }

    return flags;
}

bool ChannelView::isPaused()
{
    return this->pausedTemporarily || this->pausedBySelection;
}

// void ChannelView::beginPause()
//{
//    if (this->scrollBar.isAtBottom()) {
//        this->scrollBar.setDesiredValue(this->scrollBar.getDesiredValue() - 0.001);
//        this->layoutMessages();
//    }
//}

// void ChannelView::endPause()
//{
//}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BenchmarkGuard benchmark("paint event");

    QPainter painter(this);

    painter.fillRect(rect(), this->themeManager->splits.background);

    // draw messages
    this->drawMessages(painter);
}

// if overlays is false then it draws the message, if true then it draws things such as the grey
// overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter)
{
    auto app = getApp();

    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = size_t(this->scrollBar.getCurrentValue());

    if (start >= messagesSnapshot.getLength()) {
        return;
    }

    int y = int(-(messagesSnapshot[start].get()->getHeight() *
                  (fmod(this->scrollBar.getCurrentValue(), 1))));

    messages::MessageLayout *end = nullptr;
    bool windowFocused = this->window() == QApplication::activeWindow();

    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        messages::MessageLayout *layout = messagesSnapshot[i].get();

        bool isLastMessage = false;
        if (app->settings->showLastMessageIndicator) {
            isLastMessage = this->lastReadMessage.get() == layout;
        }

        layout->paint(painter, DRAW_WIDTH, y, i, this->selection, isLastMessage, windowFocused);

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
        auto it = this->messagesOnScreen.find(messagesSnapshot[i]);
        if (it != this->messagesOnScreen.end()) {
            this->messagesOnScreen.erase(it);
        }
    }

    // delete the message buffers that aren't on screen
    for (const std::shared_ptr<messages::MessageLayout> &item : this->messagesOnScreen) {
        item->deleteBuffer();
    }

    this->messagesOnScreen.clear();

    // add all messages on screen to the map
    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        std::shared_ptr<messages::MessageLayout> layout = messagesSnapshot[i];

        this->messagesOnScreen.insert(layout);

        if (layout.get() == end) {
            break;
        }
    }
}

void ChannelView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }

    this->pausedBySelection = false;
    this->pausedTemporarily = false;

    if (this->scrollBar.isVisible()) {
        auto app = getApp();

        float mouseMultiplier = app->settings->mouseScrollMultiplier;

        float desired = this->scrollBar.getDesiredValue();
        float delta = event->delta() * 1.5 * mouseMultiplier;

        auto snapshot = this->getMessagesSnapshot();
        int snapshotLength = (int)snapshot.getLength();
        int i = std::min((int)desired, snapshotLength);

        if (delta > 0) {
            float scrollFactor = fmod(desired, 1);
            float currentScrollLeft = (int)(scrollFactor * snapshot[i]->getHeight());

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
            float scrollFactor = 1 - fmod(desired, 1);
            float currentScrollLeft = (int)(scrollFactor * snapshot[i]->getHeight());

            for (; i < snapshotLength; i++) {
                if (delta < currentScrollLeft) {
                    desired += scrollFactor * ((double)delta / currentScrollLeft);
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

        this->scrollBar.setDesiredValue(desired, true);
    }
}

void ChannelView::enterEvent(QEvent *)
{
    //    this->pause(PAUSE_TIME);
}

void ChannelView::leaveEvent(QEvent *)
{
    this->pausedTemporarily = false;
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
    std::shared_ptr<messages::MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    // is selecting
    if (this->isMouseDown) {
        this->pause(300);
        int index = layout->getSelectionIndex(relativePos);

        this->setSelection(this->selection.start, SelectionItem(messageIndex, index));

        this->queueUpdate();
    }

    // message under cursor is collapsed
    if (layout->flags & MessageLayout::Collapsed) {
        this->setCursor(Qt::PointingHandCursor);
        tooltipWidget->hide();
        return;
    }

    // check if word underneath cursor
    const messages::MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

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

    std::shared_ptr<messages::MessageLayout> layout;
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
            this->lastPressPosition = event->screenPos();
            this->isMouseDown = true;

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
            this->lastRightPressPosition = event->screenPos();
            this->isRightMouseDown = true;
        } break;

        default:;
    }

    this->update();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    // check if mouse was pressed
    if (event->button() == Qt::LeftButton) {
        if (this->isMouseDown) {
            this->isMouseDown = false;

            if (fabsf(util::distanceBetweenPoints(this->lastPressPosition, event->screenPos())) >
                15.f) {
                return;
            }
        } else {
            return;
        }
    } else if (event->button() == Qt::RightButton) {
        if (this->isRightMouseDown) {
            this->isRightMouseDown = false;

            if (fabsf(util::distanceBetweenPoints(this->lastRightPressPosition,
                                                  event->screenPos())) > 15.f) {
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

    std::shared_ptr<messages::MessageLayout> layout;
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

    const messages::MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr) {
        return;
    }

    // handle the click
    this->handleMouseClick(event, hoverLayoutElement, layout.get());
}

void ChannelView::handleMouseClick(QMouseEvent *event,
                                   const messages::MessageLayoutElement *hoveredElement,
                                   messages::MessageLayout *layout)
{
    switch (event->button()) {
        case Qt::LeftButton: {
            if (this->selecting) {
                if (this->messagesAddedSinceSelectionPause >
                    SELECTION_RESUME_SCROLLING_MSG_THRESHOLD) {
                    this->showingLatestMessages = false;
                }

                this->pausedBySelection = false;
                this->selecting = false;
                this->pauseTimeout.stop();
                this->pausedTemporarily = false;

                this->layoutMessages();
            }

            auto &link = hoveredElement->getLink();
            if (!getApp()->settings->linksDoubleClickOnly) {
                this->handleLinkClick(event, link, layout);

                this->linkClicked.invoke(link);
            }
        } break;
        case Qt::RightButton: {
            this->addContextMenuItems(hoveredElement, layout);
        } break;
        default:;
    }
}

void ChannelView::addContextMenuItems(const messages::MessageLayoutElement *hoveredElement,
                                      messages::MessageLayout *layout)
{
    const auto &creator = hoveredElement->getCreator();
    auto creatorFlags = creator.getFlags();

    static QMenu *menu = new QMenu;
    menu->clear();

    // Emote actions
    if (creatorFlags & (MessageElement::Flags::EmoteImages | MessageElement::Flags::EmojiImage)) {
        const auto &emoteElement = static_cast<const messages::EmoteElement &>(creator);

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

        menu->addAction("Open link in browser", [url] { QDesktopServices::openUrl(QUrl(url)); });
        menu->addAction("Copy link", [url] { QApplication::clipboard()->setText(url); });

        menu->addSeparator();
    }

    // Copy actions
    if (!this->selection.isEmpty()) {
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

    menu->move(QCursor::pos());
    menu->show();
    menu->raise();

    return;
}

void ChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
    auto app = getApp();

    if (app->settings->linksDoubleClickOnly) {
        std::shared_ptr<messages::MessageLayout> layout;
        QPoint relativePos;
        int messageIndex;

        if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
            return;
        }

        // message under cursor is collapsed
        if (layout->flags & MessageLayout::Collapsed) {
            return;
        }

        const messages::MessageLayoutElement *hoverLayoutElement =
            layout->getElementAt(relativePos);

        if (hoverLayoutElement == nullptr) {
            return;
        }

        auto &link = hoverLayoutElement->getLink();
        this->handleLinkClick(event, link, layout.get());
    }
}

void ChannelView::hideEvent(QHideEvent *)
{
    for (auto &layout : this->messagesOnScreen) {
        layout->deleteBuffer();
    }

    this->messagesOnScreen.clear();
}

void ChannelView::handleLinkClick(QMouseEvent *event, const messages::Link &link,
                                  messages::MessageLayout *layout)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    switch (link.type) {
        case messages::Link::UserInfo: {
            auto user = link.value;
            auto *userPopup = new UserInfoPopup;
            userPopup->setData(user, this->channel);
            userPopup->setAttribute(Qt::WA_DeleteOnClose);
            userPopup->move(event->globalPos());
            userPopup->show();

            //            this->userPopupWidget.setName(user);
            //            this->userPopupWidget.moveTo(this, event->screenPos().toPoint());
            //            this->userPopupWidget.show();
            //            this->userPopupWidget.setFocus();

            qDebug() << "Clicked " << user << "s message";
            break;
        }

        case messages::Link::Url: {
            QDesktopServices::openUrl(QUrl(link.value));
            break;
        }

        case messages::Link::UserAction: {
            QString value = link.value;
            value.replace("{user}", layout->getMessage()->loginName);
            this->channel->sendMessage(value);
        }

        default:;
    }
}

bool ChannelView::tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageLayout> &_message,
                                  QPoint &relativePos, int &index)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = this->scrollBar.getCurrentValue();

    if (start >= messagesSnapshot.getLength()) {
        return false;
    }

    int y = -(messagesSnapshot[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

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
    if (this->scrollBar.isVisible())
        return int(this->width() - 8 * this->getScale());

    return this->width();
}

}  // namespace widgets
}  // namespace chatterino
