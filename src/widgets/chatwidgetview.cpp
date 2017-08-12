#include "widgets/chatwidgetview.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "messages/message.hpp"
#include "messages/wordpart.hpp"
#include "settingsmanager.hpp"
#include "ui_accountpopupform.h"
#include "util/distancebetweenpoints.hpp"
#include "widgets/chatwidget.hpp"

#include <QDebug>
#include <QDesktopServices>
#include <QGraphicsBlurEffect>
#include <QPainter>

#include <math.h>
#include <chrono>
#include <functional>

namespace chatterino {
namespace widgets {

ChatWidgetView::ChatWidgetView(ChatWidget *_chatWidget)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
    , scrollBar(this)
    , userPopupWidget(_chatWidget->getChannelRef())
{
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    this->setMouseTracking(true);

    QObject::connect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged, this,
                     &ChatWidgetView::wordTypeMaskChanged);

    this->scrollBar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->update();

        this->layoutMessages();
    });
}

ChatWidgetView::~ChatWidgetView()
{
    QObject::disconnect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged,
                        this, &ChatWidgetView::wordTypeMaskChanged);
}

bool ChatWidgetView::layoutMessages()
{
    auto messages = this->chatWidget->getMessagesSnapshot();

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

    int start = this->scrollBar.getCurrentValue();
    int layoutWidth = this->scrollBar.isVisible() ? width() - this->scrollBar.width() : width();

    // layout the visible messages in the view
    if (messages.getLength() > start) {
        int y = -(messages[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

        for (int i = start; i < messages.getLength(); ++i) {
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

void ChatWidgetView::updateGifEmotes()
{
    this->onlyUpdateEmotes = true;
    this->update();
}

ScrollBar &ChatWidgetView::getScrollBar()
{
    return this->scrollBar;
}

void ChatWidgetView::resizeEvent(QResizeEvent *)
{
    this->scrollBar.resize(this->scrollBar.width(), height());
    this->scrollBar.move(width() - this->scrollBar.width(), 0);

    layoutMessages();

    this->update();
}

void ChatWidgetView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter _painter(this);

    _painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // only update gif emotes
    if (this->onlyUpdateEmotes) {
        this->onlyUpdateEmotes = false;

        for (const GifEmoteData &item : this->gifEmotes) {
            _painter.fillRect(item.rect, this->colorScheme.ChatBackground);

            _painter.drawPixmap(item.rect, *item.image->getPixmap());
        }

        return;
    }

    // update all messages
    this->gifEmotes.clear();

    _painter.fillRect(rect(), this->colorScheme.ChatBackground);

    // code for tesing colors
    /*
    QColor color;
    static ConcurrentMap<qreal, QImage *> imgCache;

    std::function<QImage *(qreal)> getImg = [&scheme](qreal light) {
        return imgCache.getOrAdd(light, [&scheme, &light] {
            QImage *img = new QImage(150, 50, QImage::Format_RGB32);

            QColor color;

            for (int j = 0; j < 50; j++) {
                for (qreal i = 0; i < 150; i++) {
                    color = QColor::fromHslF(i / 150.0, light, j / 50.0);

                    scheme.normalizeColor(color);

                    img->setPixelColor(i, j, color);
                }
            }

            return img;
        });
    };

    for (qreal k = 0; k < 4.8; k++) {
        auto img = getImg(k / 5);

        painter.drawImage(QRect(k * 150, 0, 150, 150), *img);
    }

    painter.fillRect(QRect(0, 9, 500, 2), QColor(0, 0, 0));*/

    auto messages = this->chatWidget->getMessagesSnapshot();

    int start = this->scrollBar.getCurrentValue();

    if (start >= messages.getLength()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getLength(); ++i) {
        messages::MessageRef *messageRef = messages[i].get();

        std::shared_ptr<QPixmap> bufferPtr = messageRef->buffer;
        QPixmap *buffer = bufferPtr.get();

        bool updateBuffer = messageRef->updateBuffer;

        if (buffer == nullptr) {
            buffer = new QPixmap(width(), messageRef->getHeight());
            bufferPtr = std::shared_ptr<QPixmap>(buffer);
            updateBuffer = true;
        }

        // update messages that have been changed
        if (updateBuffer) {
            QPainter painter(buffer);
            painter.fillRect(buffer->rect(), (messageRef->getMessage()->getCanHighlightTab())
                                                 ? this->colorScheme.ChatBackgroundHighlighted
                                                 : this->colorScheme.ChatBackground);
            for (messages::WordPart const &wordPart : messageRef->getWordParts()) {
                // image
                if (wordPart.getWord().isImage()) {
                    messages::LazyLoadedImage &lli = wordPart.getWord().getImage();

                    const QPixmap *image = lli.getPixmap();

                    if (image != nullptr) {
                        painter.drawPixmap(QRect(wordPart.getX(), wordPart.getY(),
                                                 wordPart.getWidth(), wordPart.getHeight()),
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

        _painter.drawPixmap(0, y, *buffer);

        y += messageRef->getHeight();

        if (y > height()) {
            break;
        }
    }

    for (GifEmoteData &item : this->gifEmotes) {
        _painter.fillRect(item.rect, this->colorScheme.ChatBackground);

        _painter.drawPixmap(item.rect, *item.image->getPixmap());
    }
}

void ChatWidgetView::wheelEvent(QWheelEvent *event)
{
    if (this->scrollBar.isVisible()) {
        auto mouseMultiplier = SettingsManager::getInstance().mouseScrollMultiplier.get();

        this->scrollBar.setDesiredValue(
            this->scrollBar.getDesiredValue() - event->delta() / 10.0 * mouseMultiplier, true);
    }
}

void ChatWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;

    if (!tryGetMessageAt(event->pos(), message, relativePos)) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    // int index = message->getSelectionIndex(relativePos);
    // qDebug() << index;

    messages::Word hoverWord;
    if (!message->tryGetWordPart(relativePos, hoverWord)) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (hoverWord.getLink().isValid()) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void ChatWidgetView::mousePressEvent(QMouseEvent *event)
{
    this->isMouseDown = true;
    this->lastPressPosition = event->screenPos();

    this->chatWidget->giveFocus(Qt::MouseFocusReason);
}

void ChatWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->isMouseDown) {
        // We didn't grab the mouse press, so we shouldn't be handling the mouse
        // release
        return;
    }

    this->isMouseDown = false;

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

    if (!tryGetMessageAt(event->pos(), message, relativePos)) {
        // No message at clicked position
        this->userPopupWidget.hide();
        return;
    }

    messages::Word hoverWord;

    if (!message->tryGetWordPart(relativePos, hoverWord)) {
        return;
    }

    auto &link = hoverWord.getLink();

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

bool ChatWidgetView::tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &_message,
                                     QPoint &relativePos)
{
    auto messages = this->chatWidget->getMessagesSnapshot();

    int start = this->scrollBar.getCurrentValue();

    if (start >= messages.getLength()) {
        return false;
    }

    int y = -(messages[start]->getHeight() * (fmod(this->scrollBar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getLength(); ++i) {
        auto message = messages[i];

        if (p.y() < y + message->getHeight()) {
            relativePos = QPoint(p.x(), p.y() - y);
            _message = message;
            return true;
        }

        y += message->getHeight();
    }

    return false;
}

}  // namespace widgets
}  // namespace chatterino
