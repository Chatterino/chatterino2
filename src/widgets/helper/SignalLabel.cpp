#include "widgets/helper/SignalLabel.hpp"

SignalLabel::SignalLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
}

void SignalLabel::mouseDoubleClickEvent(QMouseEvent *ev)
{
    emit this->mouseDoubleClick(ev);
}

void SignalLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseDown();
    }

    event->ignore();
}

void SignalLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseUp();
    }

    event->ignore();
}

void SignalLabel::mouseMoveEvent(QMouseEvent *event)
{
    emit this->mouseMove(event);
    event->ignore();
}
