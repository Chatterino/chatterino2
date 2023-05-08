#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/Selection.hpp"
#include "util/ThreadGuard.hpp"
#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signal.hpp>
#include <QMenu>
#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QWidget>

#include <unordered_map>
#include <unordered_set>

namespace chatterino {
enum class HighlightState;

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class MessageFlag : int64_t;
using MessageFlags = FlagsEnum<MessageFlag>;

class MessageLayout;
using MessageLayoutPtr = std::shared_ptr<MessageLayout>;

enum class MessageElementFlag : int64_t;
using MessageElementFlags = FlagsEnum<MessageElementFlag>;

class Scrollbar;
class EffectLabel;
struct Link;
class MessageLayoutElement;
class Split;
class FilterSet;
using FilterSetPtr = std::shared_ptr<FilterSet>;

enum class PauseReason {
    Mouse,
    Selection,
    DoubleClick,
    KeyboardModifier,
};

enum class FromTwitchLinkOpenChannelIn {
    Split,
    Tab,
    BrowserPlayer,
    Streamlink,
};

using SteadyClock = std::chrono::steady_clock;

class ChannelView final : public BaseWidget
{
    Q_OBJECT

public:
    enum class Context {
        None,
        UserCard,
        ReplyThread,
        Search,
    };

    explicit ChannelView(BaseWidget *parent = nullptr, Split *split = nullptr,
                         Context context = Context::None,
                         size_t messagesLimit = 1000);

    void queueUpdate();
    Scrollbar &getScrollBar();

    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    /**
     * Copies the currently selected text to the users clipboard.
     *
     * @see ::getSelectedText()
     */
    void copySelectedText();

    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void setOverrideFlags(boost::optional<MessageElementFlags> value);
    const boost::optional<MessageElementFlags> &getOverrideFlags() const;
    void updateLastReadMessage();

    /**
     * Attempts to scroll to a message in this channel.
     * @return <code>true</code> if the message was found and highlighted.
     */
    bool scrollToMessage(const MessagePtr &message);
    /**
     * Attempts to scroll to a message id in this channel.
     * @return <code>true</code> if the message was found and highlighted.
     */
    bool scrollToMessageId(const QString &id);

    /// Pausing
    bool pausable() const;
    void setPausable(bool value);
    bool paused() const;
    void pause(PauseReason reason, boost::optional<uint> msecs = boost::none);
    void unpause(PauseReason reason);

    MessageElementFlags getFlags() const;

    ChannelPtr channel();
    void setChannel(ChannelPtr channel_);

    void setFilters(const QList<QUuid> &ids);
    const QList<QUuid> getFilterIds() const;
    FilterSetPtr getFilterSet() const;

    ChannelPtr sourceChannel() const;
    void setSourceChannel(ChannelPtr sourceChannel);
    bool hasSourceChannel() const;

    LimitedQueueSnapshot<MessageLayoutPtr> &getMessagesSnapshot();
    void queueLayout();

    void clearMessages();

    Context getContext() const;

    /**
     * @brief Creates and shows a UserInfoPopup dialog
     *
     * @param userName The login name of the user
     * @param alternativePopoutChannel Optional parameter containing the channel name to use for context
     **/
    void showUserInfoPopup(const QString &userName,
                           QString alternativePopoutChannel = QString());

    /**
     * @brief This method is meant to be used when filtering out channels.
     *        It <b>must</b> return true if a message belongs in this channel.
     *        It <b>might</b> return true if a message doesn't belong in this channel.
     */
    bool mayContainMessage(const MessagePtr &message);

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;
    pajlada::Signals::NoArgSignal selectionChanged;
    pajlada::Signals::Signal<HighlightState> tabHighlightRequested;
    pajlada::Signals::NoArgSignal liveStatusChanged;
    pajlada::Signals::Signal<const Link &> linkClicked;
    pajlada::Signals::Signal<QString, FromTwitchLinkOpenChannelIn>
        openChannelIn;

protected:
    void themeChangedEvent() override;
    void scaleChangedEvent(float scale) override;

    void resizeEvent(QResizeEvent *) override;

    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * /*event*/) override;
#else
    void enterEvent(QEvent * /*event*/) override;
#endif
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
    void messagesUpdated();

    void performLayout(bool causedByScrollbar = false);
    void layoutVisibleMessages(
        const LimitedQueueSnapshot<MessageLayoutPtr> &messages);
    void updateScrollbar(const LimitedQueueSnapshot<MessageLayoutPtr> &messages,
                         bool causedByScrollbar);

    void drawMessages(QPainter &painter);
    void setSelection(const SelectionItem &start, const SelectionItem &end);
    void selectWholeMessage(MessageLayout *layout, int &messageIndex);
    void getWordBounds(MessageLayout *layout,
                       const MessageLayoutElement *element,
                       const QPoint &relativePos, int &wordStart, int &wordEnd);

    void handleMouseClick(QMouseEvent *event,
                          const MessageLayoutElement *hoveredElement,
                          MessageLayoutPtr layout);
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayoutPtr layout, QMouseEvent *event);
    void addImageContextMenuItems(const MessageLayoutElement *hoveredElement,
                                  MessageLayoutPtr layout, QMouseEvent *event,
                                  QMenu &menu);
    void addLinkContextMenuItems(const MessageLayoutElement *hoveredElement,
                                 MessageLayoutPtr layout, QMouseEvent *event,
                                 QMenu &menu);
    void addMessageContextMenuItems(const MessageLayoutElement *hoveredElement,
                                    MessageLayoutPtr layout, QMouseEvent *event,
                                    QMenu &menu);
    void addTwitchLinkContextMenuItems(
        const MessageLayoutElement *hoveredElement, MessageLayoutPtr layout,
        QMouseEvent *event, QMenu &menu);
    void addHiddenContextMenuItems(const MessageLayoutElement *hoveredElement,
                                   MessageLayoutPtr layout, QMouseEvent *event,
                                   QMenu &menu);
    void addCommandExecutionContextMenuItems(
        const MessageLayoutElement *hoveredElement, MessageLayoutPtr layout,
        QMouseEvent *event, QMenu &menu);

    int getLayoutWidth() const;
    void updatePauses();
    void unpaused();

    void enableScrolling(const QPointF &scrollStart);
    void disableScrolling();

    /**
     * Scrolls to a message layout that must be from this view.
     *
     * @param layout Must be from this channel.
     * @param messageIdx Must be an index into this channel.
     */
    void scrollToMessageLayout(MessageLayout *layout, size_t messageIdx);

    void setInputReply(const MessagePtr &message);
    void showReplyThreadPopup(const MessagePtr &message);
    bool canReplyToMessages() const;

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
    boost::optional<SteadyClock::time_point> pauseEnd_;
    int pauseScrollOffset_ = 0;
    // Keeps track how many message indices we need to offset the selection when we resume scrolling
    uint32_t pauseSelectionOffset_ = 0;

    boost::optional<MessageElementFlags> overrideFlags_;
    MessageLayoutPtr lastReadMessage_;

    ThreadGuard snapshotGuard_;
    LimitedQueueSnapshot<MessageLayoutPtr> snapshot_;

    ChannelPtr channel_ = nullptr;
    ChannelPtr underlyingChannel_ = nullptr;
    ChannelPtr sourceChannel_ = nullptr;
    Split *split_ = nullptr;

    Scrollbar *scrollBar_;
    EffectLabel *goToBottom_;
    bool showScrollBar_ = false;

    FilterSetPtr channelFilters_;

    // Returns true if message should be included
    bool shouldIncludeMessage(const MessagePtr &m) const;

    // Returns whether the scrollbar should have highlights
    bool showScrollbarHighlights() const;

    // This variable can be used to decide whether or not we should render the
    // "Show latest messages" button
    bool showingLatestMessages_ = true;
    bool enableScrollingToBottom_ = true;

    bool onlyUpdateEmotes_ = false;

    // Mouse event variables
    bool isLeftMouseDown_ = false;
    bool isRightMouseDown_ = false;
    bool isDoubleClick_ = false;
    DoubleClickSelection doubleClickSelection_;
    QPointF lastLeftPressPosition_;
    QPointF lastRightPressPosition_;
    QPointF lastDClickPosition_;
    QTimer *clickTimer_;

    bool isScrolling_ = false;
    QPointF lastMiddlePressPosition_;
    QPointF currentMousePosition_;
    QTimer scrollTimer_;

    // We're only interested in the pointer, not the contents
    MessageLayout *highlightedMessage_ = nullptr;
    QVariantAnimation highlightAnimation_;
    void setupHighlightAnimationColors();

    struct {
        QCursor neutral;
        QCursor up;
        QCursor down;
    } cursors_;

    Selection selection_;
    bool selecting_ = false;

    const Context context_;

    LimitedQueue<MessageLayoutPtr> messages_;

    pajlada::Signals::SignalHolder signalHolder_;

    // channelConnections_ will be cleared when the underlying channel of the channelview changes
    pajlada::Signals::SignalHolder channelConnections_;

    std::unordered_set<std::shared_ptr<MessageLayout>> messagesOnScreen_;

    static constexpr int leftPadding = 8;
    static constexpr int scrollbarPadding = 8;

private slots:
    void wordFlagsChanged()
    {
        queueLayout();
        update();
    }

    void scrollUpdateRequested();
};

}  // namespace chatterino
