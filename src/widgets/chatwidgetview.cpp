#include "widgets/chatwidgetview.h"
#include "channelmanager.h"
#include "colorscheme.h"
#include "messages/message.h"
#include "messages/wordpart.h"
#include "settingsmanager.h"
#include "ui_accountpopupform.h"
#include "util/distancebetweenpoints.h"
#include "widgets/chatwidget.h"

#include <QDebug>
#include <QGraphicsBlurEffect>
#include <QPainter>

#include <math.h>
#include <chrono>
#include <functional>

namespace chatterino {
namespace widgets {

ChatWidgetView::ChatWidgetView(ChatWidget *parent)
    : QWidget(parent)
    , _chatWidget(parent)
    , _scrollbar(this)
    , _userPopupWidget(_chatWidget->getChannelRef())
    , _onlyUpdateEmotes(false)
    , _mouseDown(false)
    , _lastPressPosition()
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    _scrollbar.setSmallChange(5);
    setMouseTracking(true);

    QObject::connect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged, this,
                     &ChatWidgetView::wordTypeMaskChanged);

    _scrollbar.getCurrentValueChanged().connect([this] {
        // Whenever the scrollbar value has been changed, re-render the ChatWidgetView
        this->update();
    });
}

ChatWidgetView::~ChatWidgetView()
{
    QObject::disconnect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged,
                        this, &ChatWidgetView::wordTypeMaskChanged);
}

bool ChatWidgetView::layoutMessages()
{
    auto messages = _chatWidget->getMessagesSnapshot();

    if (messages.getSize() == 0) {
        _scrollbar.setVisible(false);

        return false;
    }

    bool showScrollbar = false, redraw = false;

    // Bool indicating whether or not we were showing all messages
    // True if one of the following statements are true:
    // The scrollbar was not visible
    // The scrollbar was visible and at the bottom
    this->showingLatestMessages = this->_scrollbar.isAtBottom() || !this->_scrollbar.isVisible();

    int start = _scrollbar.getCurrentValue();
    int layoutWidth = _scrollbar.isVisible() ? width() - _scrollbar.width() : width();

    // layout the visible messages in the view
    if (messages.getSize() > start) {
        int y = -(messages[start]->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1)));

        for (int i = start; i < messages.getSize(); ++i) {
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

    for (int i = messages.getSize() - 1; i >= 0; i--) {
        auto *message = messages[i].get();

        message->layout(layoutWidth, true);

        h -= message->getHeight();

        if (h < 0) {
            _scrollbar.setLargeChange((messages.getSize() - i) + (qreal)h / message->getHeight());
            _scrollbar.setDesiredValue(_scrollbar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    _scrollbar.setVisible(showScrollbar);

    if (!showScrollbar) {
        _scrollbar.setDesiredValue(0);
    }

    _scrollbar.setMaximum(messages.getSize());

    if (showingLatestMessages && showScrollbar) {
        // If we were showing the latest messages and the scrollbar now wants to be rendered, scroll
        // to bottom
        // TODO: Do we want to check if the user is currently moving the scrollbar?
        // Perhaps also if the user scrolled with the scrollwheel in this ChatWidget in the last 0.2
        // seconds or something
        this->_scrollbar.scrollToBottom();
    }

    return redraw;
}

void ChatWidgetView::updateGifEmotes()
{
    _onlyUpdateEmotes = true;
    this->update();
}

ScrollBar *ChatWidgetView::getScrollbar()
{
    return &_scrollbar;
}

void ChatWidgetView::resizeEvent(QResizeEvent *)
{
    _scrollbar.resize(_scrollbar.width(), height());
    _scrollbar.move(width() - _scrollbar.width(), 0);

    layoutMessages();

    this->update();
}

void ChatWidgetView::paintEvent(QPaintEvent *event)
{
    QPainter _painter(this);

    _painter.setRenderHint(QPainter::SmoothPixmapTransform);

    ColorScheme &scheme = ColorScheme::getInstance();

    // only update gif emotes
    if (_onlyUpdateEmotes) {
        _onlyUpdateEmotes = false;

        for (GifEmoteData &item : _gifEmotes) {
            _painter.fillRect(item.rect, scheme.ChatBackground);

            _painter.drawPixmap(item.rect, *item.image->getPixmap());
        }

        return;
    }

    // update all messages
    _gifEmotes.clear();

    _painter.fillRect(rect(), scheme.ChatBackground);

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

    auto messages = _chatWidget->getMessagesSnapshot();

    int start = _scrollbar.getCurrentValue();

    if (start >= messages.getSize()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getSize(); ++i) {
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
            painter.fillRect(buffer->rect(), scheme.ChatBackground);

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

                    ColorScheme::getInstance().normalizeColor(color);

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
                    GifEmoteData data;
                    data.image = &lli;
                    QRect rect(wordPart.getX(), wordPart.getY() + y, wordPart.getWidth(),
                               wordPart.getHeight());

                    data.rect = rect;

                    _gifEmotes.push_back(data);
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

    for (GifEmoteData &item : _gifEmotes) {
        _painter.fillRect(item.rect, scheme.ChatBackground);

        _painter.drawPixmap(item.rect, *item.image->getPixmap());
    }
}

void ChatWidgetView::wheelEvent(QWheelEvent *event)
{
    if (_scrollbar.isVisible()) {
        auto mouseMultiplier = SettingsManager::getInstance().mouseScrollMultiplier.get();

        _scrollbar.setDesiredValue(
            _scrollbar.getDesiredValue() - event->delta() / 10.0 * mouseMultiplier, true);
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
    _mouseDown = true;
    _lastPressPosition = event->screenPos();
}

void ChatWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!_mouseDown) {
        // We didn't grab the mouse press, so we shouldn't be handling the mouse
        // release
        return;
    }

    _mouseDown = false;

    float distance = util::distanceBetweenPoints(_lastPressPosition, event->screenPos());

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
        _userPopupWidget.hide();
        return;
    }

    messages::Word hoverWord;

    if (!message->tryGetWordPart(relativePos, hoverWord)) {
        return;
    }

    auto &link = hoverWord.getLink();

    switch (link.getType()) {
        case messages::Link::UserInfo:
            auto user = message->getMessage()->getUserName();
            _userPopupWidget.setName(user);
            _userPopupWidget.move(event->screenPos().toPoint());
            _userPopupWidget.show();
            _userPopupWidget.setFocus();

            qDebug() << "Clicked " << user << "s message";
            break;
    }
}

bool ChatWidgetView::tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &_message,
                                     QPoint &relativePos)
{
    auto messages = _chatWidget->getMessagesSnapshot();

    int start = _scrollbar.getCurrentValue();

    if (start >= messages.getSize()) {
        return false;
    }

    int y = -(messages[start]->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getSize(); ++i) {
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
