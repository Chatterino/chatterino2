#pragma once

#include "channel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/messageref.hpp"
#include "messages/word.hpp"
#include "widgets/accountpopup.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/scrollbar.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QWheelEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {

struct SelectionItem {
    int messageIndex;
    int charIndex;

    SelectionItem(int _messageIndex, int _charIndex)
    {
        this->messageIndex = _messageIndex;
        this->charIndex = _charIndex;
    }

    bool isSmallerThan(SelectionItem &other)
    {
        return messageIndex < other.messageIndex ||
               (messageIndex == other.messageIndex && charIndex < other.charIndex);
    }
};

class ChatWidget;

class ChatWidgetView : public BaseWidget
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

    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageRef> &message,
                         QPoint &relativePos, int &index);

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    void drawMessages(QPainter &painter);
    void updateMessageBuffer(messages::MessageRef *messageRef, QPixmap *buffer, int messageIndex);
    void setSelection(SelectionItem start, SelectionItem end);

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

    SelectionItem selectionStart;
    SelectionItem selectionEnd;
    SelectionItem selectionMin;
    SelectionItem selectionMax;
    bool selecting = false;

private slots:
    void wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
