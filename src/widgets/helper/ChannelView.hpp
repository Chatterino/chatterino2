#pragma once

#include "common/Channel.hpp"
#include "messages/Image.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/MessageElement.hpp"
#include "messages/Selection.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/helper/RippleEffectLabel.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

#include <unordered_set>

namespace chatterino {

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
    void setOverrideFlags(boost::optional<MessageElementFlags> value);
    const boost::optional<MessageElementFlags> &getOverrideFlags() const;
    void pause(int msecTimeout);
    void updateLastReadMessage();

    void setChannel(ChannelPtr channel_);
    LimitedQueueSnapshot<MessageLayoutPtr> getMessagesSnapshot();
    void layoutMessages();

    void clearMessages();

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;
    pajlada::Signals::NoArgSignal selectionChanged;
    pajlada::Signals::Signal<HighlightState> tabHighlightRequested;
    pajlada::Signals::Signal<const Link &> linkClicked;

protected:
    void themeChangedEvent() override;

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

    void handleLinkClick(QMouseEvent *event, const Link &link,
                         MessageLayout *layout);

    bool tryGetMessageAt(QPoint p, std::shared_ptr<MessageLayout> &message,
                         QPoint &relativePos, int &index);

private:
    void updatePauseStatus();
    void detachChannel();
    void actuallyLayoutMessages(bool causedByScollbar = false);

    void drawMessages(QPainter &painter);
    void setSelection(const SelectionItem &start, const SelectionItem &end);
    MessageElementFlags getFlags() const;
    bool isPaused();

    void handleMouseClick(QMouseEvent *event,
                          const MessageLayoutElement *hoverLayoutElement,
                          MessageLayout *layout);
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayout *layout);
    int getLayoutWidth() const;

    QTimer *layoutCooldown_;
    bool layoutQueued_;

    QTimer updateTimer_;
    bool updateQueued_ = false;
    bool messageWasAdded_ = false;
    bool lastMessageHasAlternateBackground_ = false;

    bool pausedTemporarily_ = false;
    bool pausedBySelection_ = false;
    bool pausedByScrollingUp_ = false;
    int messagesAddedSinceSelectionPause_ = 0;

    QTimer pauseTimeout_;
    boost::optional<MessageElementFlags> overrideFlags_;
    MessageLayoutPtr lastReadMessage_;

    LimitedQueueSnapshot<MessageLayoutPtr> snapshot_;

    ChannelPtr channel_;

    Scrollbar scrollBar_;
    RippleEffectLabel *goToBottom_;

    // This variable can be used to decide whether or not we should render the
    // "Show latest messages" button
    bool showingLatestMessages_ = true;
    bool enableScrollingToBottom_ = true;

    bool onlyUpdateEmotes_ = false;

    // Mouse event variables
    bool isMouseDown_ = false;
    bool isRightMouseDown_ = false;
    QPointF lastPressPosition_;
    QPointF lastRightPressPosition_;

    Selection selection_;
    bool selecting_ = false;

    LimitedQueue<MessageLayoutPtr> messages;

    pajlada::Signals::Connection messageAppendedConnection_;
    pajlada::Signals::Connection messageAddedAtStartConnection_;
    pajlada::Signals::Connection messageRemovedConnection_;
    pajlada::Signals::Connection messageReplacedConnection_;
    pajlada::Signals::Connection repaintGifsConnection_;
    pajlada::Signals::Connection layoutConnection_;

    std::vector<pajlada::Signals::ScopedConnection> connections_;
    std::vector<pajlada::Signals::ScopedConnection> channelConnections_;

    std::unordered_set<std::shared_ptr<MessageLayout>> messagesOnScreen_;

private slots:
    void wordFlagsChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace chatterino
