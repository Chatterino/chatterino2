#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QWidget>
#include "QPaintEvent"
#include "channel.h"
#include "scrollbar.h"

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetView();

    Channel *
    channel()
    {
        return m_channel;
    }

protected:
    void resizeEvent(QResizeEvent *);

private:
    ScrollBar scrollbar;
    Channel *m_channel;

    void paintEvent(QPaintEvent *);
};

#endif  // CHATVIEW_H
