#include "widgets/helper/SignalLabel.hpp"

namespace chatterino {

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
    if (event->button() == Qt::LeftButton)
    {
        emit leftMouseDown();
    }

    event->ignore();
}

void SignalLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit leftMouseUp();
    }

    event->ignore();
}

void SignalLabel::mouseMoveEvent(QMouseEvent *event)
{
    emit this->mouseMove(event);
    event->ignore();
}

}  // namespace chatterino
