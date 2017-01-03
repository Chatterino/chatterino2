#include "chatwidget.h"
#include "QPainter"
#include "QFont"
#include "QFontDatabase"
#include "QVBoxLayout"
#include "colorscheme.h"

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      vbox(this)
{
    vbox.setSpacing(0);
    vbox.setMargin(1);

    vbox.addWidget(&header);
    vbox.addWidget(&view);
    vbox.addWidget(&input);
}

ChatWidget::~ChatWidget()
{

}

void ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter (this);

    painter.fillRect(rect(), ColorScheme::getInstance().ChatBackground);
}
