#include "widgets/chatwidgetinput.h"
#include "colorscheme.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput()
{
    setFixedHeight(38);
}

void
ChatWidgetInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::instance().ChatInputBackground);
    painter.setPen(ColorScheme::instance().ChatInputBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}
}
}
