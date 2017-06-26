#include "widgets/chatwidgetheaderbutton.hpp"
#include "colorscheme.hpp"
#include "widgets/chatwidgetheader.hpp"

#include <QBrush>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetHeaderButton::ChatWidgetHeaderButton(BaseWidget *parent, int spacing)
    : BaseWidget(parent)
    , mouseOver(false)
    , mouseDown(false)
{
    setLayout(&this->ui.hbox);

    this->ui.label.setAlignment(Qt::AlignCenter);

    this->ui.hbox.setMargin(0);
    this->ui.hbox.addSpacing(spacing);
    this->ui.hbox.addWidget(&this->ui.label);
    this->ui.hbox.addSpacing(spacing);

    QObject::connect(&this->ui.label, &SignalLabel::mouseUp, this,
                     &ChatWidgetHeaderButton::labelMouseUp);
    QObject::connect(&this->ui.label, &SignalLabel::mouseDown, this,
                     &ChatWidgetHeaderButton::labelMouseDown);
}

void ChatWidgetHeaderButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QBrush brush(this->colorScheme.isLightTheme() ? QColor(0, 0, 0, 32)
                                                  : QColor(255, 255, 255, 32));

    if (mouseDown) {
        painter.fillRect(rect(), brush);
    }

    if (mouseOver) {
        painter.fillRect(rect(), brush);
    }
}

void ChatWidgetHeaderButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseDown = true;

        update();
    }
}

void ChatWidgetHeaderButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseDown = false;

        update();

        emit clicked();
    }
}

void ChatWidgetHeaderButton::enterEvent(QEvent *)
{
    mouseOver = true;

    update();
}

void ChatWidgetHeaderButton::leaveEvent(QEvent *)
{
    mouseOver = false;

    update();
}

void ChatWidgetHeaderButton::labelMouseUp()
{
    mouseDown = false;

    update();

    emit clicked();
}

void ChatWidgetHeaderButton::labelMouseDown()
{
    mouseDown = true;

    update();
}

}  // namespace widgets
}  // namespace chatterino
