#include "chatwidgetheaderbuttonlabel.h"

ChatWidgetHeaderButtonLabel::ChatWidgetHeaderButtonLabel()
{
}

void
ChatWidgetHeaderButtonLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseDown();
    }
}

void
ChatWidgetHeaderButtonLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseUp();
    }
}
