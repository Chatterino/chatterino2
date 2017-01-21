#include "widgets/chatwidgetinput.h"
#include "colorscheme.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetInput::ChatWidgetInput()
    : hbox()
    , edit()
{
    this->setLayout(&hbox);

    this->setMaximumHeight(150);

    this->hbox.addWidget(&edit);

    edit.setStyleSheet(ColorScheme::instance().InputStyleSheet);
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
