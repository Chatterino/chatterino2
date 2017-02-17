#include "widgets/chatwidgetview.h"
#include "channels.h"
#include "colorscheme.h"
#include "messages/message.h"
#include "messages/wordpart.h"
#include "settings.h"
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
    , chatWidget(parent)
    , scrollbar(this)
    , onlyUpdateEmotes(false)
{
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    this->scrollbar.setSmallChange(5);
    this->setMouseTracking(true);

    QObject::connect(&Settings::getInstance(), &Settings::wordTypeMaskChanged,
                     this, &ChatWidgetView::wordTypeMaskChanged);

    this->scrollbar.getCurrentValueChanged().connect([this] { update(); });
}

ChatWidgetView::~ChatWidgetView()
{
    QObject::disconnect(&Settings::getInstance(),
                        &Settings::wordTypeMaskChanged, this,
                        &ChatWidgetView::wordTypeMaskChanged);
}

bool
ChatWidgetView::layoutMessages()
{
    auto messages = chatWidget->getMessagesSnapshot();

    if (messages.getLength() == 0) {
        this->scrollbar.setVisible(false);

        return false;
    }

    bool showScrollbar = false, redraw = false;

    int start = this->scrollbar.getCurrentValue();

    // layout the visible messages in the view
    if (messages.getLength() > start) {
        int y = -(messages[start].get()->getHeight() *
                  (fmod(this->scrollbar.getCurrentValue(), 1)));

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
            this->scrollbar.setLargeChange((messages.getLength() - i) +
                                           (qreal)h / message->getHeight());
            this->scrollbar.setDesiredValue(this->scrollbar.getDesiredValue());

            showScrollbar = true;
            break;
        }
    }

    this->scrollbar.setVisible(showScrollbar);

    if (!showScrollbar) {
        this->scrollbar.setDesiredValue(0);
    }

    this->scrollbar.setMaximum(messages.getLength());

    return redraw;
}

void
ChatWidgetView::resizeEvent(QResizeEvent *)
{
    this->scrollbar.resize(this->scrollbar.width(), height());
    this->scrollbar.move(width() - this->scrollbar.width(), 0);

    layoutMessages();

    update();
}

void
ChatWidgetView::paintEvent(QPaintEvent *event)
{
    QPainter _painter(this);

    _painter.setRenderHint(QPainter::SmoothPixmapTransform);

    ColorScheme &scheme = ColorScheme::getInstance();

    // only update gif emotes
    if (onlyUpdateEmotes) {
        onlyUpdateEmotes = false;

        for (GifEmoteData &item : this->gifEmotes) {
            _painter.fillRect(item.rect, scheme.ChatBackground);

            _painter.drawPixmap(item.rect, *item.image->getPixmap());
        }

        return;
    }

    // update all messages
    gifEmotes.clear();

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

    auto messages = chatWidget->getMessagesSnapshot();

    int start = this->scrollbar.getCurrentValue();

    if (start >= messages.getLength()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() *
              (fmod(this->scrollbar.getCurrentValue(), 1)));

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

            for (messages::WordPart const &wordPart :
                 messageRef->getWordParts()) {
                // image
                if (wordPart.getWord().isImage()) {
                    messages::LazyLoadedImage &lli =
                        wordPart.getWord().getImage();

                    const QPixmap *image = lli.getPixmap();

                    if (image != NULL) {
                        painter.drawPixmap(
                            QRect(wordPart.getX(), wordPart.getY(),
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

                    painter.drawText(
                        QRectF(wordPart.getX(), wordPart.getY(), 10000, 10000),
                        wordPart.getText(),
                        QTextOption(Qt::AlignLeft | Qt::AlignTop));
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
                    QRect rect(wordPart.getX(), wordPart.getY() + y,
                               wordPart.getWidth(), wordPart.getHeight());

                    data.rect = rect;

                    gifEmotes.push_back(data);
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
        _painter.fillRect(item.rect, scheme.ChatBackground);

        _painter.drawPixmap(item.rect, *item.image->getPixmap());
    }
}

void
ChatWidgetView::wheelEvent(QWheelEvent *event)
{
    if (this->scrollbar.isVisible()) {
        this->scrollbar.setDesiredValue(
            this->scrollbar.getDesiredValue() -
                event->delta() / 10.0 *
                    Settings::getInstance().mouseScrollMultiplier.get(),
            true);
    }
}

void
ChatWidgetView::mouseMoveEvent(QMouseEvent *event)
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

    qDebug() << index;

    if (hoverWord.getLink().getIsValid()) {
        this->setCursor(Qt::PointingHandCursor);
        qDebug() << hoverWord.getLink().getValue();
    } else {
        this->setCursor(Qt::ArrowCursor);
    }
}

bool
ChatWidgetView::tryGetMessageAt(QPoint p,
                                std::shared_ptr<messages::MessageRef> &_message,
                                QPoint &relativePos)
{
    auto messages = chatWidget->getMessagesSnapshot();

    int start = this->scrollbar.getCurrentValue();

    if (start >= messages.getLength()) {
        return false;
    }

    int y = -(messages[start].get()->getHeight() *
              (fmod(this->scrollbar.getCurrentValue(), 1))) +
            12;

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
}
}
