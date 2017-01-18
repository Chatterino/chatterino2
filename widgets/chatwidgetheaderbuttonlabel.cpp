#include "widgets/chatwidgetheaderbuttonlabel.h"

namespace chatterino {
namespace widgets {

ChatWidgetHeaderButtonLabel::ChatWidgetHeaderButtonLabel()
{
    setAlignment(Qt::AlignCenter);
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
}
}
