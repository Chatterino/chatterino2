#ifndef CHATWIDGETHEADERBUTTONLABEL_H
#define CHATWIDGETHEADERBUTTONLABEL_H

#include <QLabel>
#include <QMouseEvent>

class ChatWidgetHeaderButtonLabel : public QLabel
{
    Q_OBJECT

public:
    ChatWidgetHeaderButtonLabel();

signals:
    void mouseDown();
    void mouseUp();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};

#endif  // CHATWIDGETHEADERBUTTONLABEL_H
