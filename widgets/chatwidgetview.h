#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "messages/lazyloadedimage.h"
#include "messages/messageref.h"
#include "messages/word.h"
#include "widgets/scrollbar.h"
#include "widgets/userpopupwidget.h"

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

    void
    updateGifEmotes()
    {
        this->onlyUpdateEmotes = true;
        this->update();
    }

protected:
    void resizeEvent(QResizeEvent *);

    void paintEvent(QPaintEvent *);
    void wheelEvent(QWheelEvent *event);

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    bool tryGetMessageAt(QPoint p,
                         std::shared_ptr<messages::MessageRef> &message,
                         QPoint &relativePos);

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    std::vector<GifEmoteData> gifEmotes;

    ChatWidget *chatWidget;

    ScrollBar scrollbar;

    UserPopupWidget userPopupWidget;
    bool onlyUpdateEmotes = false;

    // Mouse event variables
    bool mouseDown = false;
    QPointF latestPressPosition;

private slots:
    void
    wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};
}
}

#endif  // CHATVIEW_H
