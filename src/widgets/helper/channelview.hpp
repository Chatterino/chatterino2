#pragma once

#include "channel.hpp"
#include "messages/image.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageelement.hpp"
#include "messages/selection.hpp"
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
    virtual ~ChannelView();

    void queueUpdate();
    Scrollbar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void pause(int msecTimeout);
    void updateLastReadMessage();

    void setChannel(ChannelPtr channel);
    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> getMessagesSnapshot();
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

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageLayout> &message,
                         QPoint &relativePos, int &index);

private:
    QTimer updateTimer;
    bool updateQueued = false;
    bool messageWasAdded = false;
    bool paused = false;
    QTimer pauseTimeout;
    messages::MessageLayoutPtr lastReadMessage;

    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> snapshot;

    void detachChannel();
    void actuallyLayoutMessages();

    void drawMessages(QPainter &painter);
    void setSelection(const messages::SelectionItem &start, const messages::SelectionItem &end);
    messages::MessageElement::Flags getFlags() const;

    ChannelPtr channel;

    Scrollbar scrollBar;
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

    messages::LimitedQueue<messages::MessageLayoutPtr> messages;

    boost::signals2::connection messageAppendedConnection;
    boost::signals2::connection messageAddedAtStartConnection;
    boost::signals2::connection messageRemovedConnection;
    boost::signals2::connection messageReplacedConnection;
    boost::signals2::connection repaintGifsConnection;
    boost::signals2::connection layoutConnection;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    std::unordered_set<std::shared_ptr<messages::MessageLayout>> messagesOnScreen;

private slots:
    void wordFlagsChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
