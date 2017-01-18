#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "scrollbar.h"

#include <QPaintEvent>
#include <QWidget>

class ChatWidget;

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetView(ChatWidget *parent);

    bool layoutMessages();

protected:
    void resizeEvent(QResizeEvent *);

    void paintEvent(QPaintEvent *);

private:
    ChatWidget *chatWidget;

    ScrollBar scrollbar;
};

#endif  // CHATVIEW_H
