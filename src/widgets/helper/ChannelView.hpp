#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/MessageFlag.hpp"
#include "messages/Selection.hpp"
#include "util/ThreadGuard.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/TooltipWidget.hpp"

#include <pajlada/signals/signal.hpp>
#include <QGestureEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPointer>
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

class LinkInfo;

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

    /// Creates a channel view without a split.
    /// In such a view, usercards and reply-threads can't be opened.
    ///
    /// @param parent The parent of this widget. Can be `nullptr`.
    /// @param context The context in which this view is shown (e.g. as a usercard).
    /// @param messagesLimit The maximum amount of messages this view will display.
    explicit ChannelView(QWidget *parent, Context context = Context::None,
                         size_t messagesLimit = 1000);

    /// Creates a channel view in a split.
    ///
    /// @param parent The parent of this widget.
    /// @param split The split containing this widget.
    ///              @a split must be in the widget tree of @a parent.
    /// @param context The context in which this view is shown (e.g. as a usercard).
    /// @param messagesLimit The maximum amount of messages this view will display.
    explicit ChannelView(QWidget *parent, Split *split,
                         Context context = Context::None,
                         size_t messagesLimit = 1000);

    void queueUpdate();
    void queueUpdate(const QRect &area);
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
    void setOverrideFlags(std::optional<MessageElementFlags> value);
    const std::optional<MessageElementFlags> &getOverrideFlags() const;
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
    void pause(PauseReason reason,
               std::optional<uint32_t> msecs = std::nullopt);
    void unpause(PauseReason reason);

    MessageElementFlags getFlags() const;

    /// @brief The virtual channel used to display messages
    ///
    /// This channel contains all messages in this view and respects the
    /// filter settings. It will always be of type Channel, not TwitchChannel
    /// nor IrcChannel.
    /// It's **not** equal to the channel passed in #setChannel().
    /// @see #underlyingChannel()
    ChannelPtr channel() const;

    /// @brief The channel this view displays messages for
    ///
    /// This channel potentially contains more messages than visible in this
    /// view due to filter settings.
    /// It's equal to the channel passed in #setChannel().
    /// @see #channel()
    ChannelPtr underlyingChannel() const;

    /// @brief Set the channel this view is displaying
    ///
    /// @see #underlyingChannel()
    void setChannel(const ChannelPtr &underlyingChannel);

    void setFilters(const QList<QUuid> &ids);
    QList<QUuid> getFilterIds() const;
    FilterSetPtr getFilterSet() const;

    /// @brief The channel this is derived from
    ///
    /// In case of "nested" channel views such as in user popups,
    /// this channel is set to the original channel the messages came from,
    /// which is used to open user popups from this view.
    /// It's not always set.
    /// @see #hasSourceChannel()
    ChannelPtr sourceChannel() const;
    /// Setter for #sourceChannel()
    void setSourceChannel(ChannelPtr sourceChannel);
    /// Checks if this view has a #sourceChannel
    bool hasSourceChannel() const;

    LimitedQueueSnapshot<MessageLayoutPtr> &getMessagesSnapshot();

    void queueLayout();
    void invalidateBuffers();

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

    void updateColorTheme();

    /// @brief Adjusts the colors this view uses
    ///
    /// If @a isOverlay is true, the overlay colors (as specified in the theme)
    /// will be used. Otherwise, regular message-colors will be used.
    void setIsOverlay(bool isOverlay);

    Scrollbar *scrollbar();

    using ChannelViewID = std::size_t;
    ///
    /// \brief Get the ID of this ChannelView
    ///
    /// The ID is made of the underlying channel's name
    /// combined with the filter set IDs
    ChannelViewID getID() const;

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

    void resizeEvent(QResizeEvent * /*event*/) override;

    void paintEvent(QPaintEvent * /*event*/) override;
    void wheelEvent(QWheelEvent *event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * /*event*/) override;
#else
    void enterEvent(QEvent * /*event*/) override;
#endif
    void leaveEvent(QEvent * /*event*/) override;

    bool event(QEvent *event) override;
    bool gestureEvent(const QGestureEvent *event);

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void hideEvent(QHideEvent * /*event*/) override;
    void showEvent(QShowEvent *event) override;

    void handleLinkClick(QMouseEvent *event, const Link &link,
                         MessageLayout *layout);

    bool tryGetMessageAt(QPoint p, std::shared_ptr<MessageLayout> &message,
                         QPoint &relativePos, int &index);

private:
    struct InternalCtor {
    };

    ChannelView(InternalCtor tag, QWidget *parent, Split *split,
                Context context, size_t messagesLimit);

    void initializeLayout();
    void initializeScrollbar();
    void initializeSignals();

    void messageAppended(MessagePtr &message,
                         std::optional<MessageFlags> overridingFlags);
    void messageAddedAtStart(std::vector<MessagePtr> &messages);
    void messageRemoveFromStart(MessagePtr &message);
    void messageReplaced(size_t hint, const MessagePtr &prev,
                         const MessagePtr &replacement);
    void messagesUpdated();

    void performLayout(bool causedByScrollbar = false,
                       bool causedByShow = false);
    void layoutVisibleMessages(
        const LimitedQueueSnapshot<MessageLayoutPtr> &messages);
    void updateScrollbar(const LimitedQueueSnapshot<MessageLayoutPtr> &messages,
                         bool causedByScrollbar, bool causedByShow);

    void drawMessages(QPainter &painter, const QRect &area);
    void setSelection(const SelectionItem &start, const SelectionItem &end);
    void setSelection(const Selection &newSelection);
    void selectWholeMessage(MessageLayout *layout, int &messageIndex);

    void handleMouseClick(QMouseEvent *event,
                          const MessageLayoutElement *hoveredElement,
                          MessageLayoutPtr layout);
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayoutPtr layout, QMouseEvent *event);
    void addMessageContextMenuItems(QMenu *menu,
                                    const MessageLayoutPtr &layout);
    void addTwitchLinkContextMenuItems(
        QMenu *menu, const MessageLayoutElement *hoveredElement);
    void addCommandExecutionContextMenuItems(QMenu *menu,
                                             const MessageLayoutPtr &layout);

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

    void updateID();
    ChannelViewID id_{};

    bool layoutQueued_ = false;
    bool bufferInvalidationQueued_ = false;

    bool lastMessageHasAlternateBackground_ = false;
    bool lastMessageHasAlternateBackgroundReverse_ = true;

    /// Tracks the area of animated elements in the last full repaint.
    /// If this is empty (QRect::isEmpty()), no animated element is shown.
    QRect animationArea_;

    bool pausable_ = false;
    QTimer pauseTimer_;
    std::unordered_map<PauseReason, std::optional<SteadyClock::time_point>>
        pauses_;
    std::optional<SteadyClock::time_point> pauseEnd_;
    int pauseScrollMinimumOffset_ = 0;
    int pauseScrollMaximumOffset_ = 0;
    // Keeps track how many message indices we need to offset the selection when we resume scrolling
    uint32_t pauseSelectionOffset_ = 0;

    std::optional<MessageElementFlags> overrideFlags_;
    MessageLayoutPtr lastReadMessage_;

    ThreadGuard snapshotGuard_;
    LimitedQueueSnapshot<MessageLayoutPtr> snapshot_;

    /// @brief The backing (internal) channel
    ///
    /// This is a "virtual" channel where all filtered messages from
    /// @a underlyingChannel_ are added to. It contains messages visible on
    /// screen and will always be a @a Channel, or, it will never be a
    /// TwitchChannel or IrcChannel, however, it will have the same type and
    /// name as @a underlyingChannel_. It's not know to any registry/server.
    ChannelPtr channel_;

    /// @brief The channel receiving messages
    ///
    /// This channel is the one passed in #setChannel(). It's known to the
    /// respective registry (e.g. TwitchIrcServer). For Twitch channels for
    /// example, this will be an instance of TwitchChannel. This channel might
    /// contain more messages than visible if filters are active.
    ChannelPtr underlyingChannel_ = nullptr;

    /// @brief The channel @a underlyingChannel_ is derived from
    ///
    /// In case of "nested" channel views such as in user popups,
    /// this channel is set to the original channel the messages came from,
    /// which is used to open user popups from this view.
    ///
    /// @see #sourceChannel()
    /// @see #hasSourceChannel()
    ChannelPtr sourceChannel_ = nullptr;
    Split *split_;

    Scrollbar *scrollBar_;
    EffectLabel *goToBottom_{};
    bool showScrollBar_ = false;

    FilterSetPtr channelFilters_;

    // Returns true if message should be included
    bool shouldIncludeMessage(const MessagePtr &message) const;

    // Returns whether the scrollbar should have highlights
    bool showScrollbarHighlights() const;

    // This variable can be used to decide whether or not we should render the
    // "Show latest messages" button
    bool showingLatestMessages_ = true;
    bool enableScrollingToBottom_ = true;

    bool onlyUpdateEmotes_ = false;

    bool isOverlay_ = false;

    // Mouse event variables
    bool isLeftMouseDown_ = false;
    bool isRightMouseDown_ = false;
    bool isDoubleClick_ = false;
    QPointF lastLeftPressPosition_;
    QPointF lastRightPressPosition_;
    QPointF lastDoubleClickPosition_;
    QTimer clickTimer_;

    bool isScrolling_ = false;
    bool isPanning_ = false;
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
    Selection doubleClickSelection_;

    const Context context_;

    LimitedQueue<MessageLayoutPtr> messages_;

    pajlada::Signals::SignalHolder signalHolder_;

    // channelConnections_ will be cleared when the underlying channel of the channelview changes
    pajlada::Signals::SignalHolder channelConnections_;

    std::unordered_set<std::shared_ptr<MessageLayout>> messagesOnScreen_;

    MessageColors messageColors_;
    MessagePreferences messagePreferences_;

    void scrollUpdateRequested();

    TooltipWidget *const tooltipWidget_{};

    /// Pointer to a link info that hasn't loaded yet
    QPointer<LinkInfo> pendingLinkInfo_;

    /// @brief Sets the tooltip to contain the link info
    ///
    /// If the info isn't loaded yet, it's tracked until it's resolved or errored.
    void setLinkInfoTooltip(LinkInfo *info);

    /// Slot for the LinkInfo::stateChanged signal.
    void pendingLinkInfoStateChanged();
};

}  // namespace chatterino
