#include "chatwidgetinput.h"
#include "colorscheme.h"
#include "QPainter"

ChatWidgetInput::ChatWidgetInput()
{
    setFixedHeight(38);
}

void ChatWidgetInput::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::getInstance().ChatInputBackground);
    painter.setPen(ColorScheme::getInstance().ChatInputBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}
