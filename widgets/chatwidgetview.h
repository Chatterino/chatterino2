#ifndef CHATVIEW_H
#define CHATVIEW_H

#include "channel.h"
#include "messages/lazyloadedimage.h"
#include "messages/messageref.h"
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

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    std::vector<GifEmoteData> gifEmotes;

    ChatWidget *chatWidget;

    ScrollBar scrollbar;
    bool onlyUpdateEmotes;

private slots:
    void
    wordTypeMaskChanged()
    {
        update();
    }
};
}
}

#endif  // CHATVIEW_H
