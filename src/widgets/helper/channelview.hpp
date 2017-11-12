#pragma once

#include "channel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageref.hpp"
#include "messages/word.hpp"
#include "widgets/accountpopup.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/scrollbar.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

struct SelectionItem {
    int messageIndex;
    int charIndex;

    SelectionItem()
    {
        messageIndex = charIndex = 0;
    }

    SelectionItem(int _messageIndex, int _charIndex)
    {
        this->messageIndex = _messageIndex;
        this->charIndex = _charIndex;
    }

    bool isSmallerThan(const SelectionItem &other) const
    {
        return this->messageIndex < other.messageIndex ||
               (this->messageIndex == other.messageIndex && this->charIndex < other.charIndex);
    }

    bool equals(const SelectionItem &other) const
    {
        return this->messageIndex == other.messageIndex && this->charIndex == other.charIndex;
    }
};

struct Selection {
    SelectionItem start;
    SelectionItem end;
    SelectionItem min;
    SelectionItem max;

    Selection()
    {
    }

    Selection(const SelectionItem &start, const SelectionItem &end)
        : start(start)
        , end(end)
        , min(start)
        , max(end)
    {
        if (max.isSmallerThan(min)) {
            std::swap(this->min, this->max);
        }
    }

    bool isEmpty() const
    {
        return this->start.equals(this->end);
    }

    bool isSingleMessage() const
    {
        return this->min.messageIndex == this->max.messageIndex;
    }
};

class ChannelView : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(WindowManager &windowManager, BaseWidget *parent = 0);
    ~ChannelView();

    void updateGifEmotes();
    void queueUpdate();
    ScrollBar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();

    void setChannel(std::shared_ptr<Channel> channel);
    messages::LimitedQueueSnapshot<messages::SharedMessageRef> getMessagesSnapshot();
    void layoutMessages();

    void clearMessages();

    boost::signals2::signal<void(QMouseEvent *)> mouseDown;
    boost::signals2::signal<void()> selectionChanged;

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

    WindowManager &windowManager;
    QTimer updateTimer;
    bool updateQueued = false;

    void detachChannel();
    void actuallyLayoutMessages();

    void drawMessages(QPainter &painter);
    void updateMessageBuffer(messages::MessageRef *messageRef, QPixmap *buffer, int messageIndex);
    void drawMessageSelection(QPainter &painter, messages::MessageRef *messageRef, int messageIndex,
                              int bufferHeight);
    void setSelection(const SelectionItem &start, const SelectionItem &end);

    std::shared_ptr<Channel> channel;

    std::vector<GifEmoteData> gifEmotes;

    ScrollBar scrollBar;
    RippleEffectLabel *goToBottom;

    // This variable can be used to decide whether or not we should render the "Show latest
    // messages" button
    bool showingLatestMessages = true;

    AccountPopupWidget userPopupWidget;
    bool onlyUpdateEmotes = false;

    // Mouse event variables
    bool isMouseDown = false;
    QPointF lastPressPosition;

    Selection selection;
    bool selecting = false;

    messages::LimitedQueue<messages::SharedMessageRef> messages;

    boost::signals2::connection messageAppendedConnection;
    boost::signals2::connection messageRemovedConnection;
    boost::signals2::connection repaintGifsConnection;
    boost::signals2::connection layoutConnection;

private slots:
    void wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
