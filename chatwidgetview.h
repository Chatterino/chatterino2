#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QWidget>
#include "scrollbar.h"

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetView();

protected:
    void resizeEvent(QResizeEvent *);

private:
    ScrollBar scrollbar;
};

#endif // CHATVIEW_H
