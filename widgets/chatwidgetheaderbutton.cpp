#include "widgets/chatwidgetheaderbutton.h"
#include "colorscheme.h"

#include <QBrush>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetHeaderButton::ChatWidgetHeaderButton(int spacing)
    : QWidget()
    , hbox()
    , label()
    , mouseOver(false)
    , mouseDown(false)
{
    setLayout(&hbox);

    label.setAlignment(Qt::AlignCenter);

    hbox.setMargin(0);
    hbox.addSpacing(spacing);
    hbox.addWidget(&this->label);
    hbox.addSpacing(spacing);

    QObject::connect(&this->label, &SignalLabel::mouseUp, this,
                     &ChatWidgetHeaderButton::labelMouseUp);
    QObject::connect(&this->label, &SignalLabel::mouseDown, this,
                     &ChatWidgetHeaderButton::labelMouseDown);
}

void
ChatWidgetHeaderButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QBrush brush(ColorScheme::instance().IsLightTheme
                     ? QColor(0, 0, 0, 32)
                     : QColor(255, 255, 255, 32));

    if (this->mouseDown) {
        painter.fillRect(rect(), brush);
    }

    if (this->mouseOver) {
        painter.fillRect(rect(), brush);
    }
}

void
ChatWidgetHeaderButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->mouseDown = true;

        repaint();
    }
}

void
ChatWidgetHeaderButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->mouseDown = false;

        repaint();

        emit clicked();
    }
}

void
ChatWidgetHeaderButton::enterEvent(QEvent *)
{
    this->mouseOver = true;

    repaint();
}

void
ChatWidgetHeaderButton::leaveEvent(QEvent *)
{
    this->mouseOver = false;

    repaint();
}

void
ChatWidgetHeaderButton::labelMouseUp()
{
    this->mouseDown = false;

    repaint();

    emit clicked();
}

void
ChatWidgetHeaderButton::labelMouseDown()
{
    this->mouseDown = true;

    repaint();
}
}
}
