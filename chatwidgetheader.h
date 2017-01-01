#ifndef CHATWIDGETHEADER_H
#define CHATWIDGETHEADER_H

#include "QWidget"
#include "QPaintEvent"
#include "QPoint"
#include "QMouseEvent"

class ChatWidget;

class ChatWidgetHeader : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetHeader();
    ChatWidget* getChatWidget();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint dragStart;
    bool dragging = false;
};

#endif // CHATWIDGETHEADER_H
