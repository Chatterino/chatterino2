#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Selection.hpp"
#include "widgets/BaseWidget.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

#include <unordered_map>
#include <unordered_set>

namespace chatterino {
enum class HighlightState;

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class MessageFlag : uint16_t;
using MessageFlags = FlagsEnum<MessageFlag>;

class MessageLayout;
using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

enum class MessageElementFlag;
using MessageElementFlags = FlagsEnum<MessageElementFlag>;

class Scrollbar;
class EffectLabel;
struct Link;
class MessageLayoutElement;

enum class PauseReason {
    Mouse,
    Selection,
    DoubleClick,
};

using SteadyClock = std::chrono::steady_clock;

class ChannelView final : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(BaseWidget *parent = nullptr);

    void queueUpdate();
    Scrollbar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void setOverrideFlags(boost::optional<MessageElementFlags> value);
    const boost::optional<MessageElementFlags> &getOverrideFlags() const;
    void updateLastReadMessage();

    /// Pausing
    bool pausable() const;
    void setPausable(bool value);
    bool paused() const;
    void pause(PauseReason reason, boost::optional<uint> msecs = boost::none);
    void unpause(PauseReason reason);

    ChannelPtr channel();
    void setChannel(ChannelPtr channel_);

    LimitedQueueSnapshot<MessageLayoutPtr> getMessagesSnapshot();
    void queueLayout();

    void clearMessages();
    void showUserInfoPopup(const QString &userName);

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;
    pajlada::Signals::NoArgSignal selectionChanged;
    pajlada::Signals::Signal<HighlightState> tabHighlightRequested;
    pajlada::Signals::NoArgSignal liveStatusChanged;
    pajlada::Signals::Signal<const Link &> linkClicked;
    pajlada::Signals::Signal<QString> joinToChannel;

protected:
    void themeChangedEvent() override;
    void scaleChangedEvent(float scale) override;

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
    void initializeLayout();
    void initializeScrollbar();
    void initializeSignals();

    void messageAppended(MessagePtr &message,
                         boost::optional<MessageFlags> overridingFlags);
    void messageAddedAtStart(std::vector<MessagePtr> &messages);
    void messageRemoveFromStart(MessagePtr &message);
    void messageReplaced(size_t index, MessagePtr &replacement);

    void performLayout(bool causedByScollbar = false);
    void layoutVisibleMessages(
        LimitedQueueSnapshot<MessageLayoutPtr> &messages);
    void updateScrollbar(LimitedQueueSnapshot<MessageLayoutPtr> &messages,
                         bool causedByScrollbar);

    void drawMessages(QPainter &painter);
    void setSelection(const SelectionItem &start, const SelectionItem &end);
    MessageElementFlags getFlags() const;
    void selectWholeMessage(MessageLayout *layout, int &messageIndex);
    void getWordBounds(MessageLayout *layout,
                       const MessageLayoutElement *element,
                       const QPoint &relativePos, int &wordStart, int &wordEnd);

    void handleMouseClick(QMouseEvent *event,
                          const MessageLayoutElement *hoverLayoutElement,
                          MessageLayout *layout);
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayout *layout);
    int getLayoutWidth() const;
    void updatePauseTimer();

    QTimer *layoutCooldown_;
    bool layoutQueued_;

    QTimer updateTimer_;
    bool updateQueued_ = false;
    bool messageWasAdded_ = false;
    bool lastMessageHasAlternateBackground_ = false;
    bool lastMessageHasAlternateBackgroundReverse_ = true;

    bool pausable_ = false;
    QTimer pauseTimer_;
    std::unordered_map<PauseReason, boost::optional<SteadyClock::time_point>>
        pauses_;
    boost::optional<SteadyClock::time_point> pauseEnd;

    boost::optional<MessageElementFlags> overrideFlags_;
    MessageLayoutPtr lastReadMessage_;

    LimitedQueueSnapshot<MessageLayoutPtr> snapshot_;

    ChannelPtr channel_;

    Scrollbar *scrollBar_;
    EffectLabel *goToBottom_;

    // This variable can be used to decide whether or not we should render the
    // "Show latest messages" button
    bool showingLatestMessages_ = true;
    bool enableScrollingToBottom_ = true;

    bool onlyUpdateEmotes_ = false;

    // Mouse event variables
    bool isMouseDown_ = false;
    bool isRightMouseDown_ = false;
    bool isDoubleClick_ = false;
    DoubleClickSelection doubleClickSelection_;
    QPointF lastPressPosition_;
    QPointF lastRightPressPosition_;
    QPointF lastDClickPosition_;
    QTimer *clickTimer_;

    Selection selection_;
    bool selecting_ = false;

    LimitedQueue<MessageLayoutPtr> messages_;

    pajlada::Signals::Connection messageAppendedConnection_;
    pajlada::Signals::Connection messageAddedAtStartConnection_;
    pajlada::Signals::Connection messageRemovedConnection_;
    pajlada::Signals::Connection messageReplacedConnection_;
    pajlada::Signals::Connection repaintGifsConnection_;
    pajlada::Signals::Connection layoutConnection_;

    std::vector<pajlada::Signals::ScopedConnection> connections_;
    std::vector<pajlada::Signals::ScopedConnection> channelConnections_;

    std::unordered_set<std::shared_ptr<MessageLayout>> messagesOnScreen_;

    static constexpr int leftPadding = 8;
    static constexpr int scrollbarPadding = 8;

private slots:
    void wordFlagsChanged()
    {
        queueLayout();
        update();
    }
};

}  // namespace chatterino
