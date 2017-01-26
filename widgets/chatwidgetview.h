#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "messages/word.h"
#include "widgets/scrollbar.h"

#include <QPaintEvent>
#include <QScroller>
#include <QWheelEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {
class ChatWidget;

class ChatWidgetView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetView(ChatWidget *parent);
    ~ChatWidgetView();

    bool layoutMessages();

    void updateScrollbar();

protected:
    void resizeEvent(QResizeEvent *);

    void paintEvent(QPaintEvent *);
    void wheelEvent(QWheelEvent *event);

    void scroll(int dx, int dy);

private:
    ChatWidget *chatWidget;

    ScrollBar scrollbar;

private slots:
    void
    wordTypeMaskChanged()
    {
        if (layoutMessages()) {
            repaint();
        }
    }
};
}
}

#endif  // CHATVIEW_H
