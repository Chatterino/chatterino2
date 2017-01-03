#include "chatwidgetview.h"

ChatWidgetView::ChatWidgetView()
    : QWidget(),
      scrollbar(this)
{

}

void ChatWidgetView::resizeEvent(QResizeEvent *)
{
    scrollbar.resize(scrollbar.width(), height());
    scrollbar.move(width() - scrollbar.width(), 0);
}
