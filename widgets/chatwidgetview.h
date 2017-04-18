#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "messages/lazyloadedimage.h"
#include "messages/messageref.h"
#include "messages/word.h"
#include "widgets/scrollbar.h"
#include "widgets/accountpopup.h"

#include <QPaintEvent>
#include <QScroller>
#include <QWheelEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {
class ChatWidget;

class ChatWidgetView : public QWidget
{
public:
    explicit ChatWidgetView(ChatWidget *parent);
    ~ChatWidgetView();

    bool layoutMessages();

    void updateGifEmotes();
    ScrollBar *getScrollbar();

protected:
    void resizeEvent(QResizeEvent *);

    void paintEvent(QPaintEvent *);
    void wheelEvent(QWheelEvent *event);

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &message,
                         QPoint &relativePos);

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    std::vector<GifEmoteData> _gifEmotes;

    ChatWidget *_chatWidget;

    ScrollBar _scrollbar;

    UserPopupWidget _userPopupWidget;
    bool _onlyUpdateEmotes;

    // Mouse event variables
    bool _mouseDown;
    QPointF _lastPressPosition;

private slots:
    void wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};
}  // namespace widgets
}  // namespace chatterino

#endif  // CHATVIEW_H
