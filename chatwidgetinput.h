#ifndef CHATWIDGETINPUT_H
#define CHATWIDGETINPUT_H

#include "QWidget"
#include "QPaintEvent"

class ChatWidgetInput : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetInput();

protected:
    void paintEvent(QPaintEvent *);
};

#endif // CHATWIDGETINPUT_H
