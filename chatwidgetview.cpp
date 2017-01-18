#include "chatwidgetview.h"
#include "channels.h"
#include "chatwidget.h"
#include "colorscheme.h"
#include "message.h"
#include "word.h"
#include "wordpart.h"

#include <QPainter>
#include <QScroller>
#include <functional>

ChatWidgetView::ChatWidgetView(ChatWidget *parent)
    : QWidget()
    , m_chatWidget(parent)
    , m_scrollbar(this)
{
    auto scroll = QScroller::scroller(this);

    scroll->scrollTo(QPointF(0, 100));
}

bool
ChatWidgetView::layoutMessages()
{
    auto c = m_chatWidget->channel();

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
    m_scrollbar.resize(m_scrollbar.width(), height());
    m_scrollbar.move(width() - m_scrollbar.width(), 0);

    layoutMessages();
}

void
ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    auto c = m_chatWidget->channel();

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

    int y = m_scrollbar.star;

    for (std::shared_ptr<Message> const &message : messages) {
        for (WordPart const &wordPart : message.get()->wordParts()) {
            painter.setPen(QColor(255, 0, 0));
            painter.drawRect(wordPart.x(), wordPart.y() + y, wordPart.width(),
                             wordPart.height());

            // image
            if (wordPart.word().isImage()) {
                LazyLoadedImage &lli = wordPart.word().getImage();

                const QPixmap *image = lli.pixmap();

                if (image != NULL) {
                    painter.drawPixmap(
                        QRect(wordPart.x(), wordPart.y() + y, wordPart.width(),
                              wordPart.height()),
                        *image);
                }
            }
            // text
            else {
                QColor color = wordPart.word().color();

                painter.setPen(color);
                painter.setFont(wordPart.word().getFont());

                painter.drawText(
                    QRectF(wordPart.x(), wordPart.y() + y, 10000, 10000),
                    wordPart.text(), QTextOption(Qt::AlignLeft | Qt::AlignTop));
            }
        }

        y += message.get()->height();
    }
}
