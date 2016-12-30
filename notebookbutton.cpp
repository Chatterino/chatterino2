#include "notebookbutton.h"
#include "QPainter"

NotebookButton::NotebookButton(QWidget *parent)
    : QWidget(parent)
{

}

void NotebookButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor background;
    QColor foreground;

    if (mouseDown)
    {
        foreground = QColor(0, 0, 0);
        background = QColor(255, 255, 255);
    }
    else if (mouseOver)
    {
        foreground = QColor(255, 255, 255);
        background = QColor(0, 0, 0);
    }
    else
    {
        foreground = QColor(0, 0, 0);
        background = QColor(255, 255, 255);
    }

    painter.fillRect(this->rect(), background);

    float h = this->height(), w = this->width();

    if (icon == IconPlus)
    {
        painter.fillRect(QRectF((h / 12) * 2 + 1, (h / 12) * 5 + 1, w - ((h / 12) * 5), (h / 12) * 1), foreground);
        painter.fillRect(QRectF((h / 12) * 5 + 1, (h / 12) * 2 + 1, (h / 12) * 1, w - ((h / 12) * 5)), foreground);
    }
    else if (icon == IconUser)
    {

    }
    else // IconSettings
    {

    }
}

void NotebookButton::mousePressEvent(QMouseEvent *)
{
    mouseDown = true;

    this->repaint();
}

void NotebookButton::mouseReleaseEvent(QMouseEvent *)
{
    mouseDown = false;

    this->repaint();
}

void NotebookButton::enterEvent(QEvent *)
{
    mouseOver = true;

    this->repaint();
}

void NotebookButton::leaveEvent(QEvent *)
{
    mouseOver = false;

    this->repaint();
}
