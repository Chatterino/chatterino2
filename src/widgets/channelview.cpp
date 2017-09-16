#include "widgets/channelview.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/message.hpp"
#include "messages/messageref.hpp"
#include "settingsmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/distancebetweenpoints.hpp"
#include "widgets/chatwidget.hpp"

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
    , userPopupWidget(std::shared_ptr<Channel>())
{
#ifndef Q_OS_MAC
//    this->setAttribute(Qt::WA_OpaquePaintEvent);
#endif
    this->setMouseTracking(true);

    QObject::connect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged, this,
                     &ChannelView::wordTypeMaskChanged);

    this->scrollBar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->update();

        this->layoutMessages();
    });
}

ChannelView::~ChannelView()
{
    QObject::disconnect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged,
                        this, &ChannelView::wordTypeMaskChanged);
}

bool ChannelView::layoutMessages()
{
    auto messages = this->getMessagesSnapshot();

    if (messages.getLength() == 0) {
        this->scrollBar.setVisible(false);
        return false;
    }

    bool showScrollbar = false;
    bool redraw = false;

    // Bool indicating whether or not we were showing all messages
    // True if one of the following statements are true:
    // The scrollbar was not visible
    // The scrollbar was visible and at the bottom
    this->showingLatestMessages = this->scrollBar.isAtBottom() || !this->scrollBar.isVisible();

    size_t start = this->scrollBar.getCurrentValue();
    int layoutWidth =
        (this->scrollBar.isVisible() ? width() - this->scrollBar.width() : width()) - 4;

    // layout the visible messages in the view
    if (messages.getLength() > start) {
        int y = -(messages[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

        for (size_t i = start; i < messages.getLength(); ++i) {
            auto message = messages[i];

            redraw |= message->layout(layoutWidth, true);

            y += message->getHeight();

            if (y >= height()) {
                break;
            }
        }
    }

    // layout the messages at the bottom to determine the scrollbar thumb size
    int h = height() - 8;

    for (std::size_t i = messages.getLength() - 1; i > 0; i--) {
        auto *message = messages[i].get();

        message->layout(layoutWidth, true);

        h -= message->getHeight();

        if (h < 0) {
            this->scrollBar.setLargeChange((messages.getLength() - i) +
                                           (qreal)h / message->getHeight());
            this->scrollBar.setDesiredValue(this->scrollBar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    this->scrollBar.setVisible(showScrollbar);

    if (!showScrollbar) {
        this->scrollBar.setDesiredValue(0);
    }

    this->scrollBar.setMaximum(messages.getLength());

    if (this->showingLatestMessages && showScrollbar) {
        // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
        // to bottom
        // TODO: Do we want to check if the user is currently moving the scrollbar?
        // Perhaps also if the user scrolled with the scrollwheel in this ChatWidget in the last 0.2
        // seconds or something
        this->scrollBar.scrollToBottom();
    }

    return redraw;
}

void ChannelView::clearMessages()
{
    // Clear all stored messages in this chat widget
    this->messages.clear();

    // Layout chat widget messages, and force an update regardless if there are no messages
    this->layoutMessages();
    this->update();
}

void ChannelView::updateGifEmotes()
{
    this->onlyUpdateEmotes = true;
    this->update();
}

ScrollBar &ChannelView::getScrollBar()
{
    return this->scrollBar;
}

QString ChannelView::getSelectedText()
{
    LimitedQueueSnapshot<SharedMessageRef> messages = this->getMessagesSnapshot();

    QString text;
    bool isSingleMessage = this->selection.isSingleMessage();

    size_t i = std::max(0, this->selection.min.messageIndex);

    int charIndex = 0;

    bool first = true;

    for (const messages::WordPart &part : messages[i]->getWordParts()) {
        int charLength = part.getCharacterLength();

        if (charIndex + charLength < this->selection.min.charIndex) {
            charIndex += charLength;
            continue;
        }

        if (first) {
            first = false;

            if (part.getWord().isText()) {
                text += part.getText().mid(this->selection.min.charIndex - charIndex);
            } else {
                text += part.getCopyText();
            }
        }

        if (isSingleMessage && charIndex + charLength >= selection.max.charIndex) {
            if (part.getWord().isText()) {
                text += part.getText().mid(0, this->selection.max.charIndex - charIndex);
            } else {
                text += part.getCopyText();
            }
            return text;
        }

        text += part.getCopyText();

        if (part.hasTrailingSpace()) {
            text += " ";
        }

        charIndex += charLength;
    }

    text += "\n";

    for (i++; i < this->selection.max.messageIndex; i++) {
        for (const messages::WordPart &part : messages[i]->getWordParts()) {
            text += part.getCopyText();

            if (part.hasTrailingSpace()) {
                text += " ";
            }
        }
        text += "\n";
    }

    charIndex = 0;

    for (const messages::WordPart &part :
         messages[this->selection.max.messageIndex]->getWordParts()) {
        int charLength = part.getCharacterLength();

        if (charIndex + charLength >= this->selection.max.charIndex) {
            if (part.getWord().isText()) {
                text += part.getText().mid(0, this->selection.max.charIndex - charIndex);
            } else {
                text += part.getCopyText();
            }

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

messages::LimitedQueueSnapshot<SharedMessageRef> ChannelView::getMessagesSnapshot()
{
    return this->messages.getSnapshot();
}

void ChannelView::setChannel(std::shared_ptr<Channel> channel)
{
    if (this->channel) {
        this->detachChannel();
    }
    this->messages.clear();

    // on new message
    this->messageAppendedConnection =
        channel->messageAppended.connect([this](SharedMessage &message) {
            SharedMessageRef deleted;

            auto messageRef = new MessageRef(message);

            if (this->messages.appendItem(SharedMessageRef(messageRef), deleted)) {
                qreal value = std::max(0.0, this->getScrollBar().getDesiredValue() - 1);

                this->getScrollBar().setDesiredValue(value, false);
            }

            layoutMessages();
            update();
        });

    // on message removed
    this->messageRemovedConnection =
        channel->messageRemovedFromStart.connect([this](SharedMessage &) {
            this->selection.min.messageIndex--;
            this->selection.max.messageIndex--;
            this->selection.start.messageIndex--;
            this->selection.end.messageIndex--;

            layoutMessages();
            update();
        });

    auto snapshot = channel->getMessageSnapshot();

    for (size_t i = 0; i < snapshot.getLength(); i++) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(snapshot[i]);

        this->messages.appendItem(SharedMessageRef(messageRef), deleted);
    }

    this->channel = channel;

    this->userPopupWidget.setChannel(channel);
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

    layoutMessages();

    this->update();
}

void ChannelView::setSelection(const SelectionItem &start, const SelectionItem &end)
{
    // selections
    this->selection = Selection(start, end);

    //    qDebug() << min.messageIndex << ":" << min.charIndex << " " << max.messageIndex << ":"
    //             << max.charIndex;
}

void ChannelView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

// only update gif emotes
#ifndef Q_OS_MAC
//    if (this->onlyUpdateEmotes) {
//        this->onlyUpdateEmotes = false;

//        for (const GifEmoteData &item : this->gifEmotes) {
//            painter.fillRect(item.rect, this->colorScheme.ChatBackground);

//            painter.drawPixmap(item.rect, *item.image->getPixmap());
//        }

//        return;
//    }
#endif

    // update all messages
    this->gifEmotes.clear();

    painter.fillRect(rect(), this->colorScheme.ChatBackground);

    // draw messages
    this->drawMessages(painter);

    // draw gif emotes
    for (GifEmoteData &item : this->gifEmotes) {
        painter.fillRect(item.rect, this->colorScheme.ChatBackground);

        painter.drawPixmap(item.rect, *item.image->getPixmap());
    }
}

void ChannelView::drawMessages(QPainter &painter)
{
    auto messages = this->getMessagesSnapshot();

    size_t start = this->scrollBar.getCurrentValue();

    if (start >= messages.getLength()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

    for (size_t i = start; i < messages.getLength(); ++i) {
        messages::MessageRef *messageRef = messages[i].get();

        std::shared_ptr<QPixmap> bufferPtr = messageRef->buffer;
        QPixmap *buffer = bufferPtr.get();

        bool updateBuffer = messageRef->updateBuffer;

        if (buffer == nullptr) {
            buffer = new QPixmap(width(), messageRef->getHeight());
            bufferPtr = std::shared_ptr<QPixmap>(buffer);
            updateBuffer = true;
        }

        updateBuffer |= this->selecting;

        // update messages that have been changed
        if (updateBuffer) {
            this->updateMessageBuffer(messageRef, buffer, i);
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

        messageRef->buffer = bufferPtr;

        //        if (buffer != nullptr) {
        painter.drawPixmap(0, y, *buffer);
        //        }

        y += messageRef->getHeight();

        if (y > height()) {
            break;
        }
    }
}

void ChannelView::updateMessageBuffer(messages::MessageRef *messageRef, QPixmap *buffer,
                                      int messageIndex)
{
    QPainter painter(buffer);

    // draw background
    // if (this->selectionMin.messageIndex <= messageIndex &&
    //    this->selectionMax.messageIndex >= messageIndex) {
    //    painter.fillRect(buffer->rect(), QColor(24, 55, 25));
    //} else {
    painter.fillRect(buffer->rect(),
                     (messageRef->getMessage()->getCanHighlightTab())
                         ? this->colorScheme.ChatBackgroundHighlighted
                         : this->colorScheme.ChatBackground);
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

            if (image != nullptr) {
                painter.drawPixmap(QRect(wordPart.getX(), wordPart.getY(), wordPart.getWidth(),
                                         wordPart.getHeight()),
                                   *image);
            }
        }
        // text
        else {
            QColor color = wordPart.getWord().getColor();

            this->colorScheme.normalizeColor(color);

            painter.setPen(color);
            painter.setFont(wordPart.getWord().getFont());

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

    QColor selectionColor(255, 255, 255, 63);

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

            std::vector<short> &characterWidth = part.getWord().getCharacterWidthCache();

            for (int j = 0; j < offset; j++) {
                rect.setLeft(rect.left() + characterWidth[j]);
            }

            if (isSingleWord) {
                int length = (this->selection.max.charIndex - charIndex) - offset;

                rect.setRight(part.getX());

                for (int j = 0; j < offset + length; j++) {
                    rect.setRight(rect.right() + characterWidth[j]);
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

                std::vector<short> &characterWidth = part.getWord().getCharacterWidthCache();

                int length = (this->selection.max.charIndex - charIndex) - offset;

                rect.setRight(part.getX());

                for (int j = 0; j < offset + length; j++) {
                    rect.setRight(rect.right() + characterWidth[j]);
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
        auto mouseMultiplier = SettingsManager::getInstance().mouseScrollMultiplier.get();

        this->scrollBar.setDesiredValue(
            this->scrollBar.getDesiredValue() - event->delta() / 10.0 * mouseMultiplier, true);
    }
}

void ChannelView::mouseMoveEvent(QMouseEvent *event)
{
    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;
    int messageIndex;

    if (!tryGetMessageAt(event->pos(), message, relativePos, messageIndex)) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (this->selecting) {
        int index = message->getSelectionIndex(relativePos);

        this->setSelection(this->selection.start, SelectionItem(messageIndex, index));

        this->repaint();
    }

    const messages::Word *hoverWord;
    if ((hoverWord = message->tryGetWordPart(relativePos)) == nullptr) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (hoverWord->getLink().isValid()) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
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

    qDebug() << "Distance: " << distance;

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
    auto messages = this->getMessagesSnapshot();

    size_t start = this->scrollBar.getCurrentValue();

    if (start >= messages.getLength()) {
        return false;
    }

    int y = -(messages[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

    for (size_t i = start; i < messages.getLength(); ++i) {
        auto message = messages[i];

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
