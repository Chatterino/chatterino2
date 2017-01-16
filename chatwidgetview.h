#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QWidget>
#include "QPaintEvent"
#include "channel.h"
#include "scrollbar.h"

class ChatWidget;

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetView(ChatWidget *parent);

    bool layoutMessages();

protected:
    void resizeEvent(QResizeEvent *);

private:
    ChatWidget *m_chatWidget;

    ScrollBar m_scrollbar;

    void paintEvent(QPaintEvent *);
};

#endif  // CHATVIEW_H
