#pragma once

#include "channel.hpp"
#include "messages/image.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageelement.hpp"
#include "messages/selection.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/scrollbar.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

#include <unordered_set>

namespace chatterino {
namespace widgets {

class ChannelView : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(BaseWidget *parent = nullptr);
    virtual ~ChannelView() override;

    void queueUpdate();
    Scrollbar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void setOverrideFlags(boost::optional<messages::MessageElement::Flags> value);
    const boost::optional<messages::MessageElement::Flags> &getOverrideFlags() const;
    void pause(int msecTimeout);
    void updateLastReadMessage();

    void setChannel(ChannelPtr channel_);
    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> getMessagesSnapshot();
    void layoutMessages();

    void clearMessages();

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;
    pajlada::Signals::NoArgSignal selectionChanged;
    pajlada::Signals::Signal<HighlightState> tabHighlightRequested;
    pajlada::Signals::Signal<const messages::Link &> linkClicked;

protected:
    void themeRefreshEvent() override;

    void resizeEvent(QResizeEvent *) override;

    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *event) override;

    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void hideEvent(QHideEvent *) override;

    void handleLinkClick(QMouseEvent *event, const messages::Link &link,
                         messages::MessageLayout *layout);

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageLayout> &message,
                         QPoint &relativePos, int &index);

private:
    QTimer *layoutCooldown_;
    bool layoutQueued_;

    QTimer updateTimer_;
    bool updateQueued_ = false;
    bool messageWasAdded_ = false;
    bool lastMessageHasAlternateBackground_ = false;

    bool pausedTemporarily_ = false;
    bool pausedBySelection_ = false;
    bool pausedByScrollingUp_ = false;
    void updatePauseStatus();
    int messagesAddedSinceSelectionPause_ = 0;

    QTimer pauseTimeout_;
    boost::optional<messages::MessageElement::Flags> overrideFlags_;
    messages::MessageLayoutPtr lastReadMessage_;

    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> snapshot_;

    void detachChannel();
    void actuallyLayoutMessages(bool causedByScollbar = false);

    void drawMessages(QPainter &painter);
    void setSelection(const messages::SelectionItem &start, const messages::SelectionItem &end);
    messages::MessageElement::Flags getFlags() const;
    bool isPaused();

    void handleMouseClick(QMouseEvent *event,
                          const messages::MessageLayoutElement *hoverLayoutElement,
                          messages::MessageLayout *layout);
    void addContextMenuItems(const messages::MessageLayoutElement *hoveredElement,
                             messages::MessageLayout *layout);

    //    void beginPause();
    //    void endPause();

    ChannelPtr channel_;

    Scrollbar scrollBar_;
    RippleEffectLabel *goToBottom_;

    // This variable can be used to decide whether or not we should render the "Show latest
    // messages" button
    bool showingLatestMessages_ = true;
    bool enableScrollingToBottom_ = true;

    bool onlyUpdateEmotes_ = false;

    // Mouse event variables
    bool isMouseDown_ = false;
    bool isRightMouseDown_ = false;
    QPointF lastPressPosition_;
    QPointF lastRightPressPosition_;

    messages::Selection selection_;
    bool selecting_ = false;

    messages::LimitedQueue<messages::MessageLayoutPtr> messages;

    pajlada::Signals::Connection messageAppendedConnection_;
    pajlada::Signals::Connection messageAddedAtStartConnection_;
    pajlada::Signals::Connection messageRemovedConnection_;
    pajlada::Signals::Connection messageReplacedConnection_;
    pajlada::Signals::Connection repaintGifsConnection_;
    pajlada::Signals::Connection layoutConnection_;

    std::vector<pajlada::Signals::ScopedConnection> connections_;
    std::vector<pajlada::Signals::ScopedConnection> channelConnections_;

    std::unordered_set<std::shared_ptr<messages::MessageLayout>> messagesOnScreen_;

    int getLayoutWidth() const;

private slots:
    void wordFlagsChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
