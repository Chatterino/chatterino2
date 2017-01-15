#include "chatwidgetheaderbutton.h"
#include "colorscheme.h"

#include <QBrush>
#include <QPainter>

ChatWidgetHeaderButton::ChatWidgetHeaderButton()
    : QWidget()
    , hbox()
    , m_label()
    , m_mouseOver(false)
    , m_mouseDown(false)
{
    setLayout(&hbox);

    hbox.setMargin(0);
    hbox.addSpacing(6);
    hbox.addWidget(&m_label);
    hbox.addSpacing(6);

    QObject::connect(&m_label, &ChatWidgetHeaderButtonLabel::mouseUp, this,
                     &ChatWidgetHeaderButton::labelMouseUp);
    QObject::connect(&m_label, &ChatWidgetHeaderButtonLabel::mouseDown, this,
                     &ChatWidgetHeaderButton::labelMouseDown);
}

void
ChatWidgetHeaderButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QBrush brush(ColorScheme::instance().IsLightTheme
                     ? QColor(0, 0, 0, 32)
                     : QColor(255, 255, 255, 32));

    if (m_mouseDown) {
        painter.fillRect(rect(), brush);
    }

    if (m_mouseOver) {
        painter.fillRect(rect(), brush);
    }
}

void
ChatWidgetHeaderButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mouseDown = true;

        repaint();
    }
}

void
ChatWidgetHeaderButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mouseDown = false;

        repaint();

        emit clicked();
    }
}

void
ChatWidgetHeaderButton::enterEvent(QEvent *)
{
    m_mouseOver = true;

    repaint();
}

void
ChatWidgetHeaderButton::leaveEvent(QEvent *)
{
    m_mouseOver = false;

    repaint();
}

void
ChatWidgetHeaderButton::labelMouseUp()
{
    m_mouseDown = false;

    repaint();

    emit clicked();
}

void
ChatWidgetHeaderButton::labelMouseDown()
{
    m_mouseDown = true;

    repaint();
}
