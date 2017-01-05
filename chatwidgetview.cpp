#include "chatwidgetview.h"
#include "QScroller"
#include "QPainter"

ChatWidgetView::ChatWidgetView()
    : QWidget(),
      scrollbar(this),
      m_channel(NULL)
{
    auto scroll = QScroller::scroller(this);

    scroll->scrollTo(QPointF(0, 100));

    m_channel = Channel::getChannel("ian678");
}

void ChatWidgetView::resizeEvent(QResizeEvent *)
{
    scrollbar.resize(scrollbar.width(), height());
    scrollbar.move(width() - scrollbar.width(), 0);
}

void ChatWidgetView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    auto c = channel();

    if (c == NULL) return;

    auto M = c->getMessagesClone();

    delete M;
}
