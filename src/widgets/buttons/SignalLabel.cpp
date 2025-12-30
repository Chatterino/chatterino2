#include "widgets/buttons/SignalLabel.hpp"

namespace chatterino {

SignalLabel::SignalLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
}

void SignalLabel::mouseDoubleClickEvent(QMouseEvent *ev)
{
    this->mouseDoubleClick(ev);
}

void SignalLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->leftMouseDown();
    }

    event->ignore();
}

void SignalLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->leftMouseUp();
    }

    event->ignore();
}

void SignalLabel::mouseMoveEvent(QMouseEvent *event)
{
    this->mouseMove(event);
    event->ignore();
}

}  // namespace chatterino
