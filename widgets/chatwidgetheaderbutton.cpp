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

    QBrush brush(ColorScheme::getInstance().IsLightTheme
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

        update();
    }
}

void
ChatWidgetHeaderButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->mouseDown = false;

        update();

        emit clicked();
    }
}

void
ChatWidgetHeaderButton::enterEvent(QEvent *)
{
    this->mouseOver = true;

    update();
}

void
ChatWidgetHeaderButton::leaveEvent(QEvent *)
{
    this->mouseOver = false;

    update();
}

void
ChatWidgetHeaderButton::labelMouseUp()
{
    this->mouseDown = false;

    update();

    emit clicked();
}

void
ChatWidgetHeaderButton::labelMouseDown()
{
    this->mouseDown = true;

    update();
}
}
}
