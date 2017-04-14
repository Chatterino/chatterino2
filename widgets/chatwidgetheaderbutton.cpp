#include "widgets/chatwidgetheaderbutton.h"
#include "colorscheme.h"

#include <QBrush>
#include <QPainter>

namespace  chatterino {
namespace  widgets {

ChatWidgetHeaderButton::ChatWidgetHeaderButton(int spacing)
    : QWidget()
    , _hbox()
    , _label()
    , _mouseOver(false)
    , _mouseDown(false)
{
    setLayout(&_hbox);

    _label.setAlignment(Qt::AlignCenter);

    _hbox.setMargin(0);
    _hbox.addSpacing(spacing);
    _hbox.addWidget(&_label);
    _hbox.addSpacing(spacing);

    QObject::connect(&_label, &SignalLabel::mouseUp, this,
                     &ChatWidgetHeaderButton::labelMouseUp);
    QObject::connect(&_label, &SignalLabel::mouseDown, this,
                     &ChatWidgetHeaderButton::labelMouseDown);
}

void ChatWidgetHeaderButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QBrush brush(ColorScheme::getInstance().IsLightTheme ? QColor(0, 0, 0, 32)
                                                         : QColor(255, 255, 255, 32));

    if (_mouseDown) {
        painter.fillRect(rect(), brush);
    }

    if (_mouseOver) {
        painter.fillRect(rect(), brush);
    }
}

void ChatWidgetHeaderButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _mouseDown = true;

        update();
    }
}

void ChatWidgetHeaderButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _mouseDown = false;

        update();

        emit clicked();
    }
}

void ChatWidgetHeaderButton::enterEvent(QEvent *)
{
    _mouseOver = true;

    update();
}

void ChatWidgetHeaderButton::leaveEvent(QEvent *)
{
    _mouseOver = false;

    update();
}

void ChatWidgetHeaderButton::labelMouseUp()
{
    _mouseDown = false;

    update();

    emit clicked();
}

void ChatWidgetHeaderButton::labelMouseDown()
{
    _mouseDown = true;

    update();
}
}
}
