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

    m_channel = Channel::getChannel("ian678");
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
        message.get()->layout(width(), true);
    }
}

void
ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    auto c = channel();

    if (c == NULL)
        return;

    auto messages = c->getMessagesClone();

    int y = 0;

    for (std::shared_ptr<Message> const &message : messages) {
        for (WordPart const &wordPart : message.get()->wordParts()) {
            // image
            if (wordPart.word().isImage()) {
                LazyLoadedImage &lli = wordPart.word().getImage();

                const QImage *image = lli.image();

                if (image != NULL) {
                    painter.drawImage(
                        QRect(wordPart.x(), wordPart.y() + y, wordPart.width(),
                              wordPart.height()),
                        *image);
                }
            }
            // text
            else {
                painter.setPen(wordPart.word().color());
                painter.setFont(wordPart.word().getFont());

                painter.drawText(wordPart.x(), wordPart.y() + y,
                                 wordPart.getText());
            }
        }

        y += message.get()->height();
    }
}
