#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "widgets/scrollbar.h"

#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {
class ChatWidget;

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetView(ChatWidget *parent);

    bool layoutMessages();

protected:
    void resizeEvent(QResizeEvent *);

    void paintEvent(QPaintEvent *);

private:
    ChatWidget *chatWidget;

    ScrollBar scrollbar;
};
}
}

#endif  // CHATVIEW_H
