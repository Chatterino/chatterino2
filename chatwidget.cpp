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

//    QFont font("Segoe UI", 15, QFont::Normal, false);
//    this->font = font;
}

ChatWidget::~ChatWidget()
{

}

void ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter (this);

    painter.fillRect(rect(), ColorScheme::getInstance().ChatBackground);

//    QColor color (255, 0, 0);

//    painter.setPen(color);

//    painter.setFont(this->font);
//    painter.drawRect(0, 0, width() - 1, height() - 1);

//    QString text = "test text";
//    painter.drawText(20, 20, text);
}
