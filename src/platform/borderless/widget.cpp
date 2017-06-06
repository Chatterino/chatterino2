#include "widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // Set a black background for funsies
    QPalette Pal(palette());
    Pal.setColor(QPalette::Background, Qt::blue);
    setAutoFillBackground(true);
    setPalette(Pal);
}
