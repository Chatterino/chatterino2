#ifndef CHATWIDGETHEADERBUTTON_H
#define CHATWIDGETHEADERBUTTON_H

#include "chatwidgetheaderbuttonlabel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QWidget>

class ChatWidgetHeaderButton : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetHeaderButton();

    ChatWidgetHeaderButtonLabel &
    label()
    {
        return m_label;
    }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    QHBoxLayout hbox;
    ChatWidgetHeaderButtonLabel m_label;

    bool m_mouseOver;
    bool m_mouseDown;

    void labelMouseUp();
    void labelMouseDown();
};

#endif  // CHATWIDGETHEADERBUTTON_H
