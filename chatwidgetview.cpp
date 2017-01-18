#include "chatwidgetview.h"
#include "channels.h"
#include "chatwidget.h"
#include "colorscheme.h"
#include "message.h"
#include "word.h"
#include "wordpart.h"

#include <math.h>
#include <QPainter>
#include <QScroller>
#include <functional>

ChatWidgetView::ChatWidgetView(ChatWidget *parent)
    : QWidget()
    , chatWidget(parent)
    , scrollbar(this)
{
    auto scroll = QScroller::scroller(this);

    scroll->scrollTo(QPointF(0, 100));
}

bool
ChatWidgetView::layoutMessages()
{
    auto c = this->chatWidget->getChannel();

    if (c == NULL)
        return false;

    auto messages = c->getMessagesClone();

    bool redraw = false;

    for (std::shared_ptr<Message> &message : messages) {
        redraw |= message.get()->layout(this->width(), true);
    }

    return redraw;
}

void
ChatWidgetView::resizeEvent(QResizeEvent *)
{
    this->scrollbar.resize(this->scrollbar.width(), height());
    this->scrollbar.move(width() - this->scrollbar.width(), 0);

    layoutMessages();
}

void
ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    auto c = this->chatWidget->getChannel();

    QColor color;

    ColorScheme &scheme = ColorScheme::instance();

    // code for tesing colors
    /*
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

    if (c == NULL)
        return;

    auto messages = c->getMessagesClone();

    int start = this->scrollbar.getValue();

    if (start >= messages.length()) {
        return;
    }

    int y = -(messages[start].get()->getHeight() *
              (fmod(this->scrollbar.getValue(), 1)));

    for (int i = start; i < messages.size(); ++i) {
        Message *message = messages[i].get();

        for (WordPart const &wordPart : message->getWordParts()) {
            painter.setPen(QColor(255, 0, 0));
            painter.drawRect(wordPart.getX(), wordPart.getY() + y,
                             wordPart.getWidth(), wordPart.getHeight());

            // image
            if (wordPart.getWord().isImage()) {
                LazyLoadedImage &lli = wordPart.getWord().getImage();

                const QPixmap *image = lli.getPixmap();

                if (image != NULL) {
                    painter.drawPixmap(
                        QRect(wordPart.getX(), wordPart.getY() + y,
                              wordPart.getWidth(), wordPart.getHeight()),
                        *image);
                }
            }
            // text
            else {
                QColor color = wordPart.getWord().getColor();

                painter.setPen(color);
                painter.setFont(wordPart.getWord().getFont());

                painter.drawText(
                    QRectF(wordPart.getX(), wordPart.getY() + y, 10000, 10000),
                    wordPart.getText(),
                    QTextOption(Qt::AlignLeft | Qt::AlignTop));
            }
        }

        y += message->getHeight();
    }
}
