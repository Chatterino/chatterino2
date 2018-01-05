#pragma once

#include "channel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageref.hpp"
#include "messages/selection.hpp"
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
#include <pajlada/signals/signal.hpp>
#include <unordered_set>

namespace chatterino {
namespace widgets {

class ChannelView : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(BaseWidget *parent = 0);
    ~ChannelView();

    void updateGifEmotes();
    void queueUpdate();
    ScrollBar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void pause(int msecTimeout);

    void setChannel(std::shared_ptr<Channel> channel);
    messages::LimitedQueueSnapshot<messages::SharedMessageRef> getMessagesSnapshot();
    void layoutMessages();

    void clearMessages();

    boost::signals2::signal<void(QMouseEvent *)> mouseDown;
    boost::signals2::signal<void()> selectionChanged;
    pajlada::Signals::NoArgSignal highlightedMessageReceived;

protected:
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

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

    QTimer updateTimer;
    bool updateQueued = false;
    bool messageWasAdded = false;
    bool paused = false;
    QTimer pauseTimeout;

    messages::LimitedQueueSnapshot<messages::SharedMessageRef> snapshot;

    void detachChannel();
    void actuallyLayoutMessages();

    void drawMessages(QPainter &painter, bool overlays);
    void updateMessageBuffer(messages::MessageRef *messageRef, QPixmap *buffer, int messageIndex);
    void drawMessageSelection(QPainter &painter, messages::MessageRef *messageRef, int messageIndex,
                              int bufferHeight);
    void setSelection(const messages::SelectionItem &start, const messages::SelectionItem &end);

    std::shared_ptr<Channel> channel;

    std::vector<GifEmoteData> gifEmotes;

    ScrollBar scrollBar;
    RippleEffectLabel *goToBottom;

    // This variable can be used to decide whether or not we should render the "Show latest
    // messages" button
    bool showingLatestMessages = true;
    bool enableScrollingToBottom = true;

    AccountPopupWidget userPopupWidget;
    bool onlyUpdateEmotes = false;

    // Mouse event variables
    bool isMouseDown = false;
    QPointF lastPressPosition;

    messages::Selection selection;
    bool selecting = false;

    messages::LimitedQueue<messages::SharedMessageRef> messages;

    boost::signals2::connection messageAppendedConnection;
    boost::signals2::connection messageAddedAtStartConnection;
    boost::signals2::connection messageRemovedConnection;
    boost::signals2::connection repaintGifsConnection;
    boost::signals2::connection layoutConnection;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    std::unordered_set<std::shared_ptr<messages::MessageRef>> messagesOnScreen;

private slots:
    void wordTypeMaskChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
