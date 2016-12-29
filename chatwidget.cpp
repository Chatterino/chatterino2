#include "chatwidget.h"
#include "QPainter"
#include "QFont"
#include "QFontDatabase"

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    QFont font("Segoe UI", 15, QFont::Normal, false);
    this->font = font;
}

void ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter (this);
    QColor color (255, 0, 0);

    painter.setBrush(color);
    painter.setPen(color);

    painter.setFont(this->font);
    painter.drawRect(5, 10, 10, 5);

    QString text = "test text";
    painter.drawText(20, 20, text);
}
