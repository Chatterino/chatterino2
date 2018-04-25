#include "channelview.hpp"
#include "debug/log.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/benchmark.hpp"
#include "util/distancebetweenpoints.hpp"
#include "widgets/split.hpp"
#include "widgets/tooltipwidget.hpp"

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

#define LAYOUT_WIDTH (this->width() - (this->scrollBar.isVisible() ? 16 : 4) * this->getScale())

using namespace chatterino::messages;
using namespace chatterino::providers::twitch;

namespace chatterino {
namespace widgets {

ChannelView::ChannelView(BaseWidget *parent)
    : BaseWidget(parent)
    , scrollBar(this)
    , userPopupWidget(std::shared_ptr<TwitchChannel>())
{
#ifndef Q_OS_MAC
//    this->setAttribute(Qt::WA_OpaquePaintEvent);
#endif
    this->setMouseTracking(true);

    QObject::connect(&singletons::SettingManager::getInstance(),
                     &singletons::SettingManager::wordFlagsChanged, this,
                     &ChannelView::wordFlagsChanged);

    this->scrollBar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->layoutMessages();
        this->goToBottom->setVisible(this->enableScrollingToBottom && this->scrollBar.isVisible() &&
                                     !this->scrollBar.isAtBottom());

        this->queueUpdate();
    });

    singletons::WindowManager &windowManager = singletons::WindowManager::getInstance();

    this->repaintGifsConnection = windowManager.repaintGifs.connect([&] { this->queueUpdate(); });
    this->layoutConnection = windowManager.layout.connect([&](Channel *channel) {
        if (channel == nullptr || this->channel.get() == channel) {
            this->layoutMessages();
        }
    });

    this->goToBottom = new RippleEffectLabel(this, 0);
    this->goToBottom->setStyleSheet("background-color: rgba(0,0,0,0.66); color: #FFF;");
    this->goToBottom->getLabel().setText("More messages below");
    this->goToBottom->setVisible(false);

    this->managedConnections.emplace_back(
        singletons::FontManager::getInstance().fontChanged.connect([this] {
            this->layoutMessages();  //
        }));

    connect(goToBottom, &RippleEffectLabel::clicked, this, [this] {
        QTimer::singleShot(180, [this] {
            this->scrollBar.scrollToBottom(singletons::SettingManager::getInstance()
                                               .enableSmoothScrollingNewMessages.getValue());
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

    //    auto e = new QResizeEvent(this->size(), this->size());
    //    this->resizeEvent(e);
    //    delete e;

    this->scrollBar.resize(this->scrollBar.width(), this->height() + 1);

    singletons::SettingManager::getInstance().showLastMessageIndicator.connect(
        [this](auto, auto) { this->update(); }, this->managedConnections);

    this->layoutCooldown = new QTimer(this);
    this->layoutCooldown->setSingleShot(true);
    this->layoutCooldown->setInterval(66);

    QObject::connect(this->layoutCooldown, &QTimer::timeout, [this] {
        if (this->layoutQueued) {
            this->layoutMessages();
            this->layoutQueued = false;
        }
    });
}

ChannelView::~ChannelView()
{
    QObject::disconnect(&singletons::SettingManager::getInstance(),
                        &singletons::SettingManager::wordFlagsChanged, this,
                        &ChannelView::wordFlagsChanged);
    this->messageAppendedConnection.disconnect();
    this->messageRemovedConnection.disconnect();
    this->repaintGifsConnection.disconnect();
    this->layoutConnection.disconnect();
    this->messageAddedAtStartConnection.disconnect();
    this->messageReplacedConnection.disconnect();
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

void ChannelView::actuallyLayoutMessages()
{
    // BENCH(timer)
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
    int layoutWidth = LAYOUT_WIDTH;

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
        this->scrollBar.setDesiredValue(0);
    }

    this->scrollBar.setMaximum(messagesSnapshot.getLength());

    // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
    // to bottom
    // TODO: Do we want to check if the user is currently moving the scrollbar?
    // Perhaps also if the user scrolled with the scrollwheel in this ChatWidget in the last 0.2
    // seconds or something
    if (this->enableScrollingToBottom && this->showingLatestMessages && showScrollbar) {
        this->scrollBar.scrollToBottom(
            this->messageWasAdded &&
            singletons::SettingManager::getInstance().enableSmoothScrollingNewMessages.getValue());
        this->messageWasAdded = false;
    }

    // MARK(timer);

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

    Selection selection = this->selection;

    if (selection.isEmpty()) {
        return result;
    }

    qDebug() << "xd >>>>";
    for (int msg = selection.selectionMin.messageIndex; msg <= selection.selectionMax.messageIndex;
         msg++) {
        MessageLayoutPtr layout = messagesSnapshot[msg];
        int from =
            msg == selection.selectionMin.messageIndex ? selection.selectionMin.charIndex : 0;
        int to = msg == selection.selectionMax.messageIndex ? selection.selectionMax.charIndex
                                                            : layout->getLastCharacterIndex() + 1;

        qDebug() << "from:" << from << ", to:" << to;

        layout->addSelectionText(result, from, to);
    }
    qDebug() << "xd <<<<";

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
    if (!this->paused) {
        this->snapshot = this->messages.getSnapshot();
    }

    return this->snapshot;
}

void ChannelView::setChannel(ChannelPtr newChannel)
{
    if (this->channel) {
        this->detachChannel();
    }

    this->messages.clear();

    // on new message
    this->messageAppendedConnection =
        newChannel->messageAppended.connect([this](MessagePtr &message) {
            MessageLayoutPtr deleted;

            auto messageRef = new MessageLayout(message);

            if (this->messages.pushBack(MessageLayoutPtr(messageRef), deleted)) {
                if (!this->paused) {
                    if (this->scrollBar.isAtBottom()) {
                        this->scrollBar.scrollToBottom();
                    } else {
                        this->scrollBar.offset(-1);
                    }
                }
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
        });

    this->messageAddedAtStartConnection =
        newChannel->messagesAddedAtStart.connect([this](std::vector<MessagePtr> &messages) {
            std::vector<MessageLayoutPtr> messageRefs;
            messageRefs.resize(messages.size());
            for (size_t i = 0; i < messages.size(); i++) {
                messageRefs.at(i) = MessageLayoutPtr(new MessageLayout(messages.at(i)));
            }

            if (!this->paused) {
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
        });

    // on message removed
    this->messageRemovedConnection =
        newChannel->messageRemovedFromStart.connect([this](MessagePtr &) {
            this->selection.selectionMin.messageIndex--;
            this->selection.selectionMax.messageIndex--;
            this->selection.start.messageIndex--;
            this->selection.end.messageIndex--;

            this->layoutMessages();
        });

    // on message replaced
    this->messageReplacedConnection =
        newChannel->messageReplaced.connect([this](size_t index, MessagePtr replacement) {
            MessageLayoutPtr newItem(new MessageLayout(replacement));

            this->scrollBar.replaceHighlight(index, replacement->getScrollBarHighlight());

            this->messages.replaceItem(this->messages.getSnapshot()[index], newItem);
            this->layoutMessages();
        });

    auto snapshot = newChannel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        MessageLayoutPtr deleted;

        auto messageRef = new MessageLayout(snapshot[i]);

        this->messages.pushBack(MessageLayoutPtr(messageRef), deleted);
    }

    this->channel = newChannel;

    this->userPopupWidget.setChannel(newChannel);
    this->layoutMessages();
    this->queueUpdate();
}

void ChannelView::detachChannel()
{
    messageAppendedConnection.disconnect();
    messageAddedAtStartConnection.disconnect();
    messageRemovedConnection.disconnect();
    messageReplacedConnection.disconnect();
}

void ChannelView::pause(int msecTimeout)
{
    this->paused = true;

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
    this->scrollBar.resize(this->scrollBar.width(), this->height());
    this->scrollBar.move(this->width() - this->scrollBar.width(), 0);

    this->goToBottom->setGeometry(0, this->height() - 32, this->width(), 32);

    this->scrollBar.raise();

    this->layoutMessages();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start, const SelectionItem &end)
{
    // selections
    this->selection = Selection(start, end);

    this->selectionChanged.invoke();
}

messages::MessageElement::Flags ChannelView::getFlags() const
{
    if (this->overrideFlags) {
        return this->overrideFlags.get();
    }

    MessageElement::Flags flags = singletons::SettingManager::getInstance().getWordFlags();

    Split *split = dynamic_cast<Split *>(this->parentWidget());

    if (split != nullptr) {
        if (split->getModerationMode()) {
            flags = (MessageElement::Flags)(flags | MessageElement::ModeratorTools);
        }
        if (this->channel == TwitchServer::getInstance().mentionsChannel) {
            flags = (MessageElement::Flags)(flags | MessageElement::ChannelName);
        }
    }

    return flags;
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BENCH(timer);

    QPainter painter(this);

    painter.fillRect(rect(), this->themeManager.splits.background);

    // draw messages
    this->drawMessages(painter);

    //    MARK(timer);
}

// if overlays is false then it draws the message, if true then it draws things such as the grey
// overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = this->scrollBar.getCurrentValue();

    if (start >= messagesSnapshot.getLength()) {
        return;
    }

    int y = -(messagesSnapshot[start].get()->getHeight() *
              (fmod(this->scrollBar.getCurrentValue(), 1)));

    messages::MessageLayout *end = nullptr;
    bool windowFocused = this->window() == QApplication::activeWindow();

    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        messages::MessageLayout *layout = messagesSnapshot[i].get();

        bool isLastMessage = false;
        if (singletons::SettingManager::getInstance().showLastMessageIndicator) {
            isLastMessage = this->lastReadMessage.get() == layout;
        }

        layout->paint(painter, y, i, this->selection, isLastMessage, windowFocused);

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
    if (this->scrollBar.isVisible()) {
        float mouseMultiplier = singletons::SettingManager::getInstance().mouseScrollMultiplier;

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
                    snapshot[i - 1]->layout(LAYOUT_WIDTH, this->getScale(), this->getFlags());
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
                    snapshot[i + 1]->layout(LAYOUT_WIDTH, this->getScale(), this->getFlags());

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
    this->paused = false;
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier)) {
        this->unsetCursor();

        event->ignore();
        return;
    }

    if (singletons::SettingManager::getInstance().pauseChatHover.getValue()) {
        this->pause(300);
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
    if (this->selecting) {
        this->pause(500);
        int index = layout->getSelectionIndex(relativePos);

        this->setSelection(this->selection.start, SelectionItem(messageIndex, index));

        this->queueUpdate();
    }

    // message under cursor is collapsed
    if (layout->getMessage()->flags & Message::Collapsed) {
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
    if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier)) {
        this->unsetCursor();

        event->ignore();
        return;
    }

    if (singletons::SettingManager::getInstance().linksDoubleClickOnly.getValue()) {
        this->pause(200);
    }

    this->isMouseDown = true;

    this->lastPressPosition = event->screenPos();

    std::shared_ptr<messages::MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    this->mouseDown.invoke(event);

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        setCursor(Qt::ArrowCursor);

        auto messagesSnapshot = this->getMessagesSnapshot();
        if (messagesSnapshot.getLength() == 0) {
            return;
        }

        // Start selection at the last message at its last index
        auto lastMessageIndex = messagesSnapshot.getLength() - 1;
        auto lastMessage = messagesSnapshot[lastMessageIndex];
        auto lastCharacterIndex = lastMessage->getLastCharacterIndex();

        SelectionItem selectionItem(lastMessageIndex, lastCharacterIndex);
        this->setSelection(selectionItem, selectionItem);
        this->selecting = true;

        return;
    }

    // check if message is collapsed
    if (layout->getMessage()->flags & Message::Collapsed) {
        return;
    }

    int index = layout->getSelectionIndex(relativePos);

    auto selectionItem = SelectionItem(messageIndex, index);
    this->setSelection(selectionItem, selectionItem);
    this->selecting = true;

    this->repaint();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->modifiers() & (Qt::AltModifier | Qt::ControlModifier)) {
        this->unsetCursor();

        event->ignore();
        return;
    }

    if (!this->isMouseDown) {
        // We didn't grab the mouse press, so we shouldn't be handling the mouse
        // release
        return;
    }

    if (this->selecting) {
        this->paused = false;
    }

    this->isMouseDown = false;
    this->selecting = false;

    float distance = util::distanceBetweenPoints(this->lastPressPosition, event->screenPos());

    if (fabsf(distance) > 15.f) {
        // It wasn't a proper click, so we don't care about that here
        return;
    }

    // If you clicked and released less than  X pixels away, it counts
    // as a click!

    this->layoutMessages();

    std::shared_ptr<messages::MessageLayout> layout;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
        // No message at clicked position
        this->userPopupWidget.hide();
        return;
    }

    // message under cursor is collapsed
    if (layout->getMessage()->flags & Message::MessageFlags::Collapsed) {
        layout->getMessage()->flags &= ~Message::MessageFlags::Collapsed;

        this->layoutMessages();
        return;
    }

    const messages::MessageLayoutElement *hoverLayoutElement = layout->getElementAt(relativePos);

    if (hoverLayoutElement == nullptr) {
        return;
    }

    auto &link = hoverLayoutElement->getLink();
    if (event->button() != Qt::LeftButton ||
        !singletons::SettingManager::getInstance().linksDoubleClickOnly) {
        this->handleLinkClick(event, link, layout.get());
    }

    this->linkClicked.invoke(link);
}

void ChannelView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (singletons::SettingManager::getInstance().linksDoubleClickOnly) {
        std::shared_ptr<messages::MessageLayout> layout;
        QPoint relativePos;
        int messageIndex;

        if (!tryGetMessageAt(event->pos(), layout, relativePos, messageIndex)) {
            return;
        }

        // message under cursor is collapsed
        if (layout->getMessage()->flags & Message::Collapsed) {
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
    switch (link.type) {
        case messages::Link::UserInfo: {
            auto user = link.value;
            this->userPopupWidget.setName(user);
            this->userPopupWidget.moveTo(this, event->screenPos().toPoint());
            this->userPopupWidget.show();
            this->userPopupWidget.setFocus();

            qDebug() << "Clicked " << user << "s message";
            break;
        }
        case messages::Link::Url: {
            if (event->button() == Qt::RightButton) {
                static QMenu *menu = nullptr;
                static QString url;

                if (menu == nullptr) {
                    menu = new QMenu;
                    menu->addAction("Open in browser",
                                    [] { QDesktopServices::openUrl(QUrl(url)); });
                    menu->addAction("Copy to clipboard",
                                    [] { QApplication::clipboard()->setText(url); });
                }

                url = link.value;
                menu->move(QCursor::pos());
                menu->show();
            } else {
                QDesktopServices::openUrl(QUrl(link.value));
            }
            break;
        }
        case messages::Link::UserAction: {
            QString value = link.value;
            value.replace("{user}", layout->getMessage()->loginName);
            this->channel->sendMessage(value);
        }
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

}  // namespace widgets
}  // namespace chatterino
