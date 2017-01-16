#include "chatwidgetview.h"
#include "channels.h"
#include "chatwidget.h"
#include "message.h"
#include "word.h"
#include "wordpart.h"

#include <QPainter>
#include <QScroller>

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
