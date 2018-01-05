#include "channelview.hpp"
#include "debug/log.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "messages/messageref.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/benchmark.hpp"
#include "util/distancebetweenpoints.hpp"
#include "widgets/split.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsBlurEffect>
#include <QPainter>

#include <math.h>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>

using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

ChannelView::ChannelView(BaseWidget *parent)
    : BaseWidget(parent)
    , scrollBar(this)
    , userPopupWidget(std::shared_ptr<twitch::TwitchChannel>())
{
#ifndef Q_OS_MAC
//    this->setAttribute(Qt::WA_OpaquePaintEvent);
#endif
    this->setMouseTracking(true);

    QObject::connect(&singletons::SettingManager::getInstance(),
                     &singletons::SettingManager::wordTypeMaskChanged, this,
                     &ChannelView::wordTypeMaskChanged);

    this->scrollBar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->layoutMessages();
        this->goToBottom->setVisible(this->enableScrollingToBottom && this->scrollBar.isVisible() &&
                                     !this->scrollBar.isAtBottom());

        this->queueUpdate();
    });

    singletons::WindowManager &windowManager = singletons::WindowManager::getInstance();

    this->repaintGifsConnection =
        windowManager.repaintGifs.connect([&] { this->updateGifEmotes(); });
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

    connect(goToBottom, &RippleEffectLabel::clicked, this,
            [this] { QTimer::singleShot(180, [this] { this->scrollBar.scrollToBottom(); }); });

    this->updateTimer.setInterval(1000 / 60);
    this->updateTimer.setSingleShot(true);
    connect(&this->updateTimer, &QTimer::timeout, this, [this] {
        if (this->updateQueued) {
            this->updateQueued = false;
            this->repaint();
            this->updateTimer.start();
        }
    });
}

ChannelView::~ChannelView()
{
    QObject::disconnect(&singletons::SettingManager::getInstance(),
                        &singletons::SettingManager::wordTypeMaskChanged, this,
                        &ChannelView::wordTypeMaskChanged);
    this->messageAppendedConnection.disconnect();
    this->messageRemovedConnection.disconnect();
    this->repaintGifsConnection.disconnect();
    this->layoutConnection.disconnect();
}

void ChannelView::queueUpdate()
{
    if (this->updateTimer.isActive()) {
        this->updateQueued = true;
        return;
    }

    this->repaint();

    this->updateTimer.start();
}

void ChannelView::layoutMessages()
{
    this->actuallyLayoutMessages();
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
    int layoutWidth =
        this->width() - (this->scrollBar.isVisible() ? 16 : 4) * this->getDpiMultiplier();

    // layout the visible messages in the view
    if (messagesSnapshot.getLength() > start) {
        int y =
            -(messagesSnapshot[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

        for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
            auto message = messagesSnapshot[i];

            redrawRequired |= message->layout(layoutWidth, this->getDpiMultiplier());

            y += message->getHeight();

            if (y >= height()) {
                break;
            }
        }
    }

    // layout the messages at the bottom to determine the scrollbar thumb size
    int h = height() - 8;

    for (std::size_t i = messagesSnapshot.getLength() - 1; i > 0; i--) {
        auto *message = messagesSnapshot[i].get();

        message->layout(layoutWidth, this->getDpiMultiplier());

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

    if (this->enableScrollingToBottom && this->showingLatestMessages && showScrollbar) {
        // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
        // to bottom
        // TODO: Do we want to check if the user is currently moving the scrollbar?
        // Perhaps also if the user scrolled with the scrollwheel in this ChatWidget in the last 0.2
        // seconds or something
        this->scrollBar.scrollToBottom();
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

void ChannelView::updateGifEmotes()
{
    if (!this->gifEmotes.empty()) {
        this->onlyUpdateEmotes = true;
        this->queueUpdate();
    }
}

ScrollBar &ChannelView::getScrollBar()
{
    return this->scrollBar;
}

QString ChannelView::getSelectedText()
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    QString text;
    bool isSingleMessage = this->selection.isSingleMessage();

    size_t i = std::max(0, this->selection.min.messageIndex);

    int charIndex = 0;

    bool first = true;

    auto addPart = [&](const WordPart &part, int from = 0, int to = -1) {
        if (part.getCopyText().isEmpty()) {
            return;
        }

        if (part.getWord().isText()) {
            text += part.getText().mid(from, to);
        } else {
            text += part.getCopyText();
        }
    };

    // first line
    for (const messages::WordPart &part : messagesSnapshot[i]->getWordParts()) {
        int charLength = part.getCharacterLength();

        if (charIndex + charLength < this->selection.min.charIndex) {
            charIndex += charLength;
            continue;
        }

        if (first) {
            first = false;
            bool isSingleWord = isSingleMessage && this->selection.max.charIndex - charIndex <
                                                       part.getCharacterLength();

            if (isSingleWord) {
                // return single word
                addPart(part, this->selection.min.charIndex - charIndex,
                        this->selection.max.charIndex - this->selection.min.charIndex);
                return text;
            } else {
                // add first word of the selection
                addPart(part, this->selection.min.charIndex - charIndex);
            }
        } else if (isSingleMessage && charIndex + charLength >= selection.max.charIndex) {
            addPart(part, 0, this->selection.max.charIndex - charIndex);

            return text;
        } else {
            text += part.getCopyText() + (part.hasTrailingSpace() ? " " : "");
        }

        charIndex += charLength;
    }

    text += "\n";

    // middle lines
    for (i++; (int)i < this->selection.max.messageIndex; i++) {
        for (const messages::WordPart &part : messagesSnapshot[i]->getWordParts()) {
            if (!part.getCopyText().isEmpty()) {
                text += part.getCopyText();

                if (part.hasTrailingSpace()) {
                    text += " ";
                }
            }
        }
        text += "\n";
    }

    // last line
    charIndex = 0;

    for (const messages::WordPart &part :
         messagesSnapshot[this->selection.max.messageIndex]->getWordParts()) {
        int charLength = part.getCharacterLength();

        if (charIndex + charLength >= this->selection.max.charIndex) {
            addPart(part, 0, this->selection.max.charIndex - charIndex);

            return text;
        }

        text += part.getCopyText();

        if (part.hasTrailingSpace()) {
            text += " ";
        }

        charIndex += charLength;
    }

    return text;
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

messages::LimitedQueueSnapshot<SharedMessageRef> ChannelView::getMessagesSnapshot()
{
    return this->messages.getSnapshot();
}

void ChannelView::setChannel(std::shared_ptr<Channel> newChannel)
{
    if (this->channel) {
        this->detachChannel();
    }
    this->messages.clear();

    // on new message
    this->messageAppendedConnection =
        newChannel->messageAppended.connect([this](SharedMessage &message) {
            SharedMessageRef deleted;

            auto messageRef = new MessageRef(message);

            if (this->messages.pushBack(SharedMessageRef(messageRef), deleted)) {
                if (this->scrollBar.isAtBottom()) {
                    this->scrollBar.scrollToBottom();
                } else {
                    this->scrollBar.offset(-1);
                }
            }

            if (message->containsHighlightedPhrase()) {
                this->highlightedMessageReceived.invoke();
            }

            layoutMessages();
        });

    this->messageAddedAtStartConnection =
        newChannel->messagesAddedAtStart.connect([this](std::vector<SharedMessage> &messages) {
            std::vector<SharedMessageRef> messageRefs;
            messageRefs.resize(messages.size());
            qDebug() << messages.size();
            for (int i = 0; i < messages.size(); i++) {
                messageRefs.at(i) = SharedMessageRef(new MessageRef(messages.at(i)));
            }

            if (this->messages.pushFront(messageRefs).size() > 0) {
                if (this->scrollBar.isAtBottom()) {
                    this->scrollBar.scrollToBottom();
                } else {
                    this->scrollBar.offset((qreal)messages.size());
                }
            }

            layoutMessages();
        });

    // on message removed
    this->messageRemovedConnection =
        newChannel->messageRemovedFromStart.connect([this](SharedMessage &) {
            this->selection.min.messageIndex--;
            this->selection.max.messageIndex--;
            this->selection.start.messageIndex--;
            this->selection.end.messageIndex--;

            this->layoutMessages();
        });

    auto snapshot = newChannel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(snapshot[i]);

        this->messages.pushBack(SharedMessageRef(messageRef), deleted);
    }

    this->channel = newChannel;

    this->userPopupWidget.setChannel(newChannel);
}

void ChannelView::detachChannel()
{
    // on message added
    this->messageAppendedConnection.disconnect();

    // on message removed
    this->messageRemovedConnection.disconnect();
}

void ChannelView::resizeEvent(QResizeEvent *)
{
    this->scrollBar.resize(this->scrollBar.width(), height());
    this->scrollBar.move(width() - this->scrollBar.width(), 0);

    this->goToBottom->setGeometry(0, this->height() - 32, this->width(), 32);

    this->scrollBar.raise();

    layoutMessages();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start, const SelectionItem &end)
{
    // selections
    this->selection = Selection(start, end);

    this->selectionChanged();

    //    qDebug() << min.messageIndex << ":" << min.charIndex << " " << max.messageIndex << ":"
    //             << max.charIndex;
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    //    BENCH(timer);
    QPainter painter(this);

// only update gif emotes
#ifndef Q_OS_MAC
//    if (this->onlyUpdateEmotes) {
//        this->onlyUpdateEmotes = false;

//        for (const GifEmoteData &item : this->gifEmotes) {
//            painter.fillRect(item.rect, this->themeManager.ChatBackground);

//            painter.drawPixmap(item.rect, *item.image->getPixmap());
//        }

//        return;
//    }
#endif

    // update all messages
    this->gifEmotes.clear();

    painter.fillRect(rect(), this->themeManager.splits.background);

    // draw messages
    this->drawMessages(painter, false);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw gif emotes
    for (GifEmoteData &item : this->gifEmotes) {
        painter.drawPixmap(item.rect, *item.image->getPixmap());
    }

    // draw the overlays of the messages (such as disabled message blur-out)
    this->drawMessages(painter, true);

    //    MARK(timer);
}

// if overlays is false then it draws the message, if true then it draws things such as the grey
// overlay when a message is disabled
void ChannelView::drawMessages(QPainter &painter, bool overlays)
{
    auto messagesSnapshot = this->getMessagesSnapshot();

    size_t start = this->scrollBar.getCurrentValue();

    if (start >= messagesSnapshot.getLength()) {
        return;
    }

    int y = -(messagesSnapshot[start].get()->getHeight() *
              (fmod(this->scrollBar.getCurrentValue(), 1)));

    messages::MessageRef *end = nullptr;

    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        messages::MessageRef *messageRef = messagesSnapshot[i].get();

        if (overlays) {
            if (messageRef->isDisabled()) {
                painter.fillRect(0, y, this->width(), messageRef->getHeight(),
                                 this->themeManager.messages.disabled);
            }
        } else {
            std::shared_ptr<QPixmap> buffer = messageRef->buffer;

            //        bool updateBuffer = messageRef->updateBuffer;
            bool updateBuffer = false;

            if (!buffer) {
                buffer = std::shared_ptr<QPixmap>(new QPixmap(width(), messageRef->getHeight()));
                updateBuffer = true;
            }

            updateBuffer |= this->selecting;

            // update messages that have been changed
            if (updateBuffer) {
                this->updateMessageBuffer(messageRef, buffer.get(), i);
                // qDebug() << "updating buffer xD";
            }

            // get gif emotes
            for (messages::WordPart const &wordPart : messageRef->getWordParts()) {
                if (wordPart.getWord().isImage()) {
                    messages::LazyLoadedImage &lli = wordPart.getWord().getImage();

                    if (lli.getAnimated()) {
                        GifEmoteData gifEmoteData;
                        gifEmoteData.image = &lli;
                        QRect rect(wordPart.getX(), wordPart.getY() + y, wordPart.getWidth(),
                                   wordPart.getHeight());

                        gifEmoteData.rect = rect;

                        this->gifEmotes.push_back(gifEmoteData);
                    }
                }
            }

            messageRef->buffer = buffer;

            if (buffer) {
                painter.drawPixmap(0, y, *buffer.get());
            }
        }

        y += messageRef->getHeight();

        end = messageRef;
        if (y > height()) {
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
    for (std::shared_ptr<messages::MessageRef> item : this->messagesOnScreen) {
        item->buffer.reset();
    }

    this->messagesOnScreen.clear();

    // add all messages on screen to the map
    for (size_t i = start; i < messagesSnapshot.getLength(); ++i) {
        std::shared_ptr<messages::MessageRef> messageRef = messagesSnapshot[i];

        this->messagesOnScreen.insert(messageRef);

        if (messageRef.get() == end) {
            break;
        }
    }
}

void ChannelView::updateMessageBuffer(messages::MessageRef *messageRef, QPixmap *buffer,
                                      int messageIndex)
{
    QPainter painter(buffer);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // draw background
    // if (this->selectionMin.messageIndex <= messageIndex &&
    //    this->selectionMax.messageIndex >= messageIndex) {
    //    painter.fillRect(buffer->rect(), QColor(24, 55, 25));
    //} else {
    painter.fillRect(buffer->rect(), (messageRef->getMessage()->containsHighlightedPhrase())
                                         ? this->themeManager.messages.backgrounds.highlighted
                                         : this->themeManager.messages.backgrounds.regular);
    //}

    // draw selection
    if (!selection.isEmpty()) {
        drawMessageSelection(painter, messageRef, messageIndex, buffer->height());
    }

    // draw message
    for (messages::WordPart const &wordPart : messageRef->getWordParts()) {
        // image
        if (wordPart.getWord().isImage()) {
            messages::LazyLoadedImage &lli = wordPart.getWord().getImage();

            const QPixmap *image = lli.getPixmap();

            if (image != nullptr && !lli.getAnimated()) {
                painter.drawPixmap(QRect(wordPart.getX(), wordPart.getY(), wordPart.getWidth(),
                                         wordPart.getHeight()),
                                   *image);
            }
        }
        // text
        else {
            QColor color = wordPart.getWord().getTextColor().getColor(this->themeManager);

            this->themeManager.normalizeColor(color);

            painter.setPen(color);
            painter.setFont(wordPart.getWord().getFont(this->getDpiMultiplier()));

            painter.drawText(QRectF(wordPart.getX(), wordPart.getY(), 10000, 10000),
                             wordPart.getText(), QTextOption(Qt::AlignLeft | Qt::AlignTop));
        }
    }

    messageRef->updateBuffer = false;
}

void ChannelView::drawMessageSelection(QPainter &painter, messages::MessageRef *messageRef,
                                       int messageIndex, int bufferHeight)
{
    if (this->selection.min.messageIndex > messageIndex ||
        this->selection.max.messageIndex < messageIndex) {
        return;
    }

    QColor selectionColor = this->themeManager.messages.selection;

    int charIndex = 0;
    size_t i = 0;
    auto &parts = messageRef->getWordParts();

    int currentLineNumber = 0;
    QRect rect;

    if (parts.size() > 0) {
        if (selection.min.messageIndex == messageIndex) {
            rect.setTop(parts.at(0).getY());
        }
        rect.setLeft(parts.at(0).getX());
    }

    // skip until selection start
    if (this->selection.min.messageIndex == messageIndex && this->selection.min.charIndex != 0) {
        for (; i < parts.size(); i++) {
            const messages::WordPart &part = parts.at(i);
            auto characterLength = part.getCharacterLength();

            if (characterLength + charIndex > selection.min.charIndex) {
                break;
            }

            charIndex += characterLength;
            currentLineNumber = part.getLineNumber();
        }

        if (i >= parts.size()) {
            return;
        }

        // handle word that has a cut of selection
        const messages::WordPart &part = parts.at(i);

        // check if selection if single word
        int characterLength = part.getCharacterLength();
        bool isSingleWord = charIndex + characterLength > this->selection.max.charIndex &&
                            this->selection.max.messageIndex == messageIndex;

        rect = part.getRect();
        currentLineNumber = part.getLineNumber();

        if (part.getWord().isText()) {
            int offset = this->selection.min.charIndex - charIndex;

            for (int j = 0; j < offset; j++) {
                rect.setLeft(rect.left() + part.getCharWidth(j, this->getDpiMultiplier()));
            }

            if (isSingleWord) {
                int length = (this->selection.max.charIndex - charIndex) - offset;

                rect.setRight(part.getX());

                for (int j = 0; j < offset + length; j++) {
                    rect.setRight(rect.right() + part.getCharWidth(j, this->getDpiMultiplier()));
                }

                painter.fillRect(rect, selectionColor);

                return;
            }
        } else {
            if (isSingleWord) {
                if (charIndex + 1 != this->selection.max.charIndex) {
                    rect.setRight(part.getX() + part.getWord().getImage().getScaledWidth());
                }
                painter.fillRect(rect, selectionColor);

                return;
            }

            if (charIndex != this->selection.min.charIndex) {
                rect.setLeft(part.getX() + part.getWord().getImage().getScaledWidth());
            }
        }

        i++;
        charIndex += characterLength;
    }

    // go through lines and draw selection
    for (; i < parts.size(); i++) {
        const messages::WordPart &part = parts.at(i);

        int charLength = part.getCharacterLength();

        bool isLastSelectedWord = this->selection.max.messageIndex == messageIndex &&
                                  charIndex + charLength > this->selection.max.charIndex;

        if (part.getLineNumber() == currentLineNumber) {
            rect.setLeft(std::min(rect.left(), part.getX()));
            rect.setTop(std::min(rect.top(), part.getY()));
            rect.setRight(std::max(rect.right(), part.getRight()));
            rect.setBottom(std::max(rect.bottom(), part.getBottom() - 1));
        } else {
            painter.fillRect(rect, selectionColor);

            currentLineNumber = part.getLineNumber();

            rect = part.getRect();
        }

        if (isLastSelectedWord) {
            if (part.getWord().isText()) {
                int offset = this->selection.min.charIndex - charIndex;

                int length = (this->selection.max.charIndex - charIndex) - offset;

                rect.setRight(part.getX());

                for (int j = 0; j < offset + length; j++) {
                    rect.setRight(rect.right() + part.getCharWidth(j, this->getDpiMultiplier()));
                }
            } else {
                if (this->selection.max.charIndex == charIndex) {
                    rect.setRight(part.getX());
                }
            }
            painter.fillRect(rect, selectionColor);

            return;
        }

        charIndex += charLength;
    }

    if (this->selection.max.messageIndex != messageIndex) {
        rect.setBottom(bufferHeight);
    }

    painter.fillRect(rect, selectionColor);
}

void ChannelView::wheelEvent(QWheelEvent *event)
{
    if (this->scrollBar.isVisible()) {
        float mouseMultiplier = singletons::SettingManager::getInstance().mouseScrollMultiplier;

        float desired = this->scrollBar.getDesiredValue();
        float delta = event->delta() * 1.5 * mouseMultiplier;

        auto snapshot = this->getMessagesSnapshot();
        int i = std::min((int)desired, (int)snapshot.getLength());

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
                    snapshot[i - 1]->layout(this->width(), this->getDpiMultiplier());
                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i - 1]->getHeight();
                }
            }
        } else {
            delta = -delta;
            float scrollFactor = 1 - fmod(desired, 1);
            float currentScrollLeft = (int)(scrollFactor * snapshot[i]->getHeight());

            for (; i < snapshot.getLength(); i++) {
                if (delta < currentScrollLeft) {
                    desired += scrollFactor * ((double)delta / currentScrollLeft);
                    break;
                } else {
                    delta -= currentScrollLeft;
                    desired += scrollFactor;
                }

                if (i == snapshot.getLength() - 1) {
                    desired = snapshot.getLength();
                } else {
                    snapshot[i + 1]->layout(this->width(), this->getDpiMultiplier());

                    scrollFactor = 1;
                    currentScrollLeft = snapshot[i + 1]->getHeight();
                }
            }
        }

        this->scrollBar.setDesiredValue(desired, true);
    }
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    auto tooltipWidget = TooltipWidget::getInstance();
    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;
    int messageIndex;

    // no message under cursor
    if (!tryGetMessageAt(event->pos(), message, relativePos, messageIndex)) {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }

    // message under cursor is collapsed
    if (message->isCollapsed()) {
        this->setCursor(Qt::PointingHandCursor);
        tooltipWidget->hide();
        return;
    }

    // is selecting
    if (this->selecting) {
        int index = message->getSelectionIndex(relativePos);

        this->setSelection(this->selection.start, SelectionItem(messageIndex, index));

        this->repaint();
    }

    // check if word underneath cursor
    const messages::Word *hoverWord;
    if ((hoverWord = message->tryGetWordPart(relativePos)) == nullptr) {
        this->setCursor(Qt::ArrowCursor);
        tooltipWidget->hide();
        return;
    }
    const auto &tooltip = hoverWord->getTooltip();

    if (hoverWord->isImage()) {
        tooltipWidget->moveTo(event->globalPos());
        tooltipWidget->setText(tooltip);
        tooltipWidget->show();
    }

    // check if word has a link
    if (hoverWord->getLink().isValid()) {
        this->setCursor(Qt::PointingHandCursor);
    } else {
        this->setCursor(Qt::ArrowCursor);
    }
}

void ChannelView::mousePressEvent(QMouseEvent *event)
{
    this->isMouseDown = true;

    this->lastPressPosition = event->screenPos();

    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;
    int messageIndex;

    this->mouseDown(event);

    if (!tryGetMessageAt(event->pos(), message, relativePos, messageIndex)) {
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
    if (message->isCollapsed()) {
        return;
    }

    int index = message->getSelectionIndex(relativePos);

    auto selectionItem = SelectionItem(messageIndex, index);
    this->setSelection(selectionItem, selectionItem);
    this->selecting = true;

    this->repaint();
}

void ChannelView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->isMouseDown) {
        // We didn't grab the mouse press, so we shouldn't be handling the mouse
        // release
        return;
    }

    this->isMouseDown = false;
    this->selecting = false;

    float distance = util::distanceBetweenPoints(this->lastPressPosition, event->screenPos());

    // qDebug() << "Distance: " << distance;

    if (fabsf(distance) > 15.f) {
        // It wasn't a proper click, so we don't care about that here
        return;
    }

    // If you clicked and released less than  X pixels away, it counts
    // as a click!

    // show user thing pajaW

    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), message, relativePos, messageIndex)) {
        // No message at clicked position
        this->userPopupWidget.hide();
        return;
    }

    // message under cursor is collapsed
    if (message->isCollapsed()) {
        message->setCollapsed(false);
        this->layoutMessages();
        return;
    }

    const messages::Word *hoverWord;

    if ((hoverWord = message->tryGetWordPart(relativePos)) == nullptr) {
        return;
    }

    auto &link = hoverWord->getLink();

    switch (link.getType()) {
        case messages::Link::UserInfo: {
            auto user = link.getValue();
            this->userPopupWidget.setName(user);
            this->userPopupWidget.move(event->screenPos().toPoint());
            this->userPopupWidget.updatePermissions();
            this->userPopupWidget.show();
            this->userPopupWidget.setFocus();

            qDebug() << "Clicked " << user << "s message";
            break;
        }
        case messages::Link::Url: {
            QDesktopServices::openUrl(QUrl(link.getValue()));
            break;
        }
    }
}

bool ChannelView::tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &_message,
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
