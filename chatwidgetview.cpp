#include "chatwidgetview.h"
#include "message.h"
#include "word.h"
#include "wordpart.h"

#include <QPainter>
#include <QScroller>

ChatWidgetView::ChatWidgetView()
    : QWidget()
    , scrollbar(this)
    , m_channel(NULL)
{
    auto scroll = QScroller::scroller(this);

    scroll->scrollTo(QPointF(0, 100));

    m_channel = Channel::getChannel("fourtf");
}

void
ChatWidgetView::resizeEvent(QResizeEvent *)
{
    scrollbar.resize(scrollbar.width(), height());
    scrollbar.move(width() - scrollbar.width(), 0);

    auto c = channel();

    if (c == NULL)
        return;

    auto messages = c->getMessagesClone();

    for (std::shared_ptr<Message> &message : messages) {
        qInfo(QString::number(width()).toStdString().c_str());
        message.get()->layout(this->width(), true);
    }
}

void
ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    auto c = channel();

    if (c == NULL)
        return;

    auto messages = c->getMessagesClone();

    int y = 0;

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
                painter.setPen(wordPart.word().color());
                painter.setFont(wordPart.word().getFont());

                painter.drawText(
                    QRectF(wordPart.x(), wordPart.y() + y, 10000, 10000),
                    wordPart.text(), QTextOption(Qt::AlignLeft | Qt::AlignTop));
            }
        }

        y += message.get()->height();
    }
}
