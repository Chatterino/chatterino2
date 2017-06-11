#pragma once

#include "channel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/messageref.hpp"
#include "messages/word.hpp"
#include "widgets/accountpopup.hpp"
#include "widgets/scrollbar.hpp"

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
    explicit ChatWidgetView(ChatWidget *_chatWidget);
    ~ChatWidgetView();

    bool layoutMessages();

    void updateGifEmotes();
    ScrollBar &getScrollBar();

protected:
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &message,
                         QPoint &relativePos);

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    std::vector<GifEmoteData> gifEmotes;

    ChatWidget *const chatWidget;

    ScrollBar scrollBar;

    // This variable can be used to decide whether or not we should render the "Show latest
    // messages" button
    bool showingLatestMessages = true;

    AccountPopupWidget userPopupWidget;
    bool onlyUpdateEmotes = false;

    // Mouse event variables
    bool isMouseDown = false;
    QPointF lastPressPosition;

private slots:
    void wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
