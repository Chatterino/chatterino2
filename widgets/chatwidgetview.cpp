#include "widgets/chatwidgetview.h"
#include "channelmanager.h"
#include "colorscheme.h"
#include "messages/message.h"
#include "messages/wordpart.h"
#include "settingsmanager.h"
#include "ui_userpopup.h"
#include "widgets/chatwidget.h"

#include <math.h>
#include <QDebug>
#include <QGraphicsBlurEffect>
#include <QPainter>
#include <chrono>
#include <functional>

namespace chatterino {
namespace widgets {

ChatWidgetView::ChatWidgetView(ChatWidget *parent)
    : QWidget()
    , _chatWidget(parent)
    , _scrollbar(this)
    , _userPopupWidget(_chatWidget->getChannel())
    , _onlyUpdateEmotes(false)
    , _mouseDown(false)
    , _lastPressPosition()
{
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    _scrollbar.setSmallChange(5);
    this->setMouseTracking(true);

    QObject::connect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged, this,
                     &ChatWidgetView::wordTypeMaskChanged);

    _scrollbar.getCurrentValueChanged().connect([this] { update(); });
}

ChatWidgetView::~ChatWidgetView()
{
    QObject::disconnect(&SettingsManager::getInstance(), &SettingsManager::wordTypeMaskChanged,
                        this, &ChatWidgetView::wordTypeMaskChanged);
}

bool ChatWidgetView::layoutMessages()
{
    auto messages = _chatWidget->getMessagesSnapshot();

    if (messages.getLength() == 0) {
        _scrollbar.setVisible(false);

        return false;
    }

    bool showScrollbar = false, redraw = false;

    int start = _scrollbar.getCurrentValue();

    // layout the visible messages in the view
    if (messages.getLength() > start) {
        int y = -(messages[start].get()->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1)));

        for (int i = start; i < messages.getLength(); ++i) {
            auto messagePtr = messages[i];
            auto message = messagePtr.get();

            redraw |= message->layout(this->width(), true);

            y += message->getHeight();

            if (y >= height()) {
                break;
            }
        }
    }

    // layout the messages at the bottom to determine the scrollbar thumb size
    int h = this->height() - 8;

    for (int i = messages.getLength() - 1; i >= 0; i--) {
        auto *message = messages[i].get();

        message->layout(this->width(), true);

        h -= message->getHeight();

        if (h < 0) {
            _scrollbar.setLargeChange((messages.getLength() - i) + (qreal)h / message->getHeight());
            _scrollbar.setDesiredValue(_scrollbar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    _scrollbar.setVisible(showScrollbar);

    if (!showScrollbar) {
        _scrollbar.setDesiredValue(0);
    }

    _scrollbar.setMaximum(messages.getLength());

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

    update();
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

    if (start >= messages.getLength()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1)));

    for (int i = start; i < messages.getLength(); ++i) {
        messages::MessageRef *messageRef = messages[i].get();

        std::shared_ptr<QPixmap> bufferPtr = messageRef->buffer;
        QPixmap *buffer = bufferPtr.get();

        bool updateBuffer = messageRef->updateBuffer;

        if (buffer == nullptr) {
            buffer = new QPixmap(this->width(), messageRef->getHeight());
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

                    if (image != NULL) {
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
        _scrollbar.setDesiredValue(
            _scrollbar.getDesiredValue() -
                event->delta() / 10.0 * SettingsManager::getInstance().mouseScrollMultiplier.get(),
            true);
    }
}

void ChatWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    std::shared_ptr<messages::MessageRef> message;
    QPoint relativePos;

    if (!tryGetMessageAt(event->pos(), message, relativePos)) {
        this->setCursor(Qt::ArrowCursor);
        return;
    }

    auto _message = message->getMessage();
    auto user = _message->getUserName();

    messages::Word hoverWord;

    if (!message->tryGetWordPart(relativePos, hoverWord)) {
        this->setCursor(Qt::ArrowCursor);
        return;
    }

    int index = message->getSelectionIndex(relativePos);

    if (hoverWord.getLink().getIsValid()) {
        this->setCursor(Qt::PointingHandCursor);
    } else {
        this->setCursor(Qt::ArrowCursor);
    }
}

void ChatWidgetView::mousePressEvent(QMouseEvent *event)
{
    _mouseDown = true;
    _lastPressPosition = event->screenPos();
}

static float distanceBetweenPoints(const QPointF &p1, const QPointF &p2)
{
    QPointF tmp = p1 - p2;

    float distance = 0.f;
    distance += tmp.x() * tmp.x();
    distance += tmp.y() * tmp.y();

    return std::sqrt(distance);
}

void ChatWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!_mouseDown) {
        // We didn't grab the mouse press, so we shouldn't be handling the mouse
        // release
        return;
    }

    _mouseDown = false;

    float distance = distanceBetweenPoints(_lastPressPosition, event->screenPos());

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

    auto _message = message->getMessage();
    auto user = _message->getUserName();

    qDebug() << "Clicked " << user << "s message";

    _userPopupWidget.setName(user);
    _userPopupWidget.move(event->screenPos().toPoint());
    _userPopupWidget.show();
    _userPopupWidget.setFocus();
}

bool ChatWidgetView::tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &_message,
                                     QPoint &relativePos)
{
    auto messages = _chatWidget->getMessagesSnapshot();

    int start = _scrollbar.getCurrentValue();

    if (start >= messages.getLength()) {
        return false;
    }

    int y = -(messages[start].get()->getHeight() * (fmod(_scrollbar.getCurrentValue(), 1))) + 12;

    for (int i = start; i < messages.getLength(); ++i) {
        auto message = messages[i];

        y += message->getHeight();

        if (p.y() < y) {
            relativePos = QPoint(p.x(), y - p.y());
            _message = message;
            return true;
        }
    }

    return false;
}
}  // namespace widgets
}  // namespace chatterino
