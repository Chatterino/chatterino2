#pragma once

#include "common/Aliases.hpp"
#include "common/Channel.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/splits/SplitCommon.hpp"

#include <boost/signals2.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QFont>
#include <QPointer>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {

class ChannelView;
class SplitHeader;
class SplitInput;
class SplitContainer;
class SplitOverlay;
class SelectChannelDialog;
class OverlayWindow;

// Each ChatWidget consists of three sub-elements that handle their own part of
// the chat widget: ChatWidgetHeader
//   - Responsible for rendering which channel the ChatWidget is in, and the
//   menu in the top-left of
//     the chat widget
// ChatWidgetView
//   - Responsible for rendering all chat messages, and the scrollbar
// ChatWidgetInput
//   - Responsible for rendering and handling user text input
//
// Each sub-element has a reference to the parent Chat Widget
class Split : public BaseWidget
{
    friend class SplitInput;

    Q_OBJECT

public:
    explicit Split(QWidget *parent);

    ~Split() override;

    pajlada::Signals::NoArgSignal channelChanged;
    pajlada::Signals::NoArgSignal focused;
    pajlada::Signals::NoArgSignal focusLost;

    ChannelView &getChannelView();
    SplitInput &getInput();

    IndirectChannel getIndirectChannel();
    ChannelPtr getChannel() const;
    void setChannel(IndirectChannel newChannel);

    void setFilters(const QList<QUuid> ids);
    const QList<QUuid> getFilters() const;

    void setModerationMode(bool value);
    bool getModerationMode() const;

    void insertTextToInput(const QString &text);

    void showChangeChannelPopup(const char *dialogTitle, bool empty,
                                std::function<void(bool)> callback);
    void updateGifEmotes();
    void updateLastReadMessage();
    void setIsTopRightSplit(bool value);

    void drag();

    bool isInContainer() const;

    void setContainer(SplitContainer *container);

    void setInputReply(const MessagePtr &reply);

    // This is called on window focus lost
    void unpause();

    OverlayWindow *overlayWindow();

    static pajlada::Signals::Signal<Qt::KeyboardModifiers>
        modifierStatusChanged;
    static Qt::KeyboardModifiers modifierStatus;

    enum class Action {
        RefreshTab,
        ResetMouseStatus,
        AppendNewSplit,
        Delete,

        SelectSplitLeft,
        SelectSplitRight,
        SelectSplitAbove,
        SelectSplitBelow,
    };

    pajlada::Signals::Signal<Action> actionRequested;
    pajlada::Signals::Signal<ChannelPtr> openSplitRequested;

    pajlada::Signals::Signal<SplitDirection, Split *> insertSplitRequested;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent * /*event*/) override;
#else
    void enterEvent(QEvent * /*event*/) override;
#endif
    void leaveEvent(QEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void channelNameUpdated(const QString &newChannelName);
    void handleModifiers(Qt::KeyboardModifiers modifiers);
    void updateInputPlaceholder();
    void addShortcuts() override;

    /**
     * @brief Opens Twitch channel stream in a browser player (opens a formatted link)
     */
    void openChannelInBrowserPlayer(ChannelPtr channel);
    /**
     * @brief Opens Twitch channel stream in streamlink app (if stream is live and streamlink is installed)
     */
    void openChannelInStreamlink(const QString channelName);
    /**
     * @brief Opens Twitch channel chat in a new Chatterino tab
     */
    void joinChannelInNewTab(ChannelPtr channel);

    /**
     * @brief Refresh moderation mode layouts/buttons
     *
     * Should be called after after the moderation mode is changed or
     * moderation actions have been changed
     **/
    void refreshModerationMode();

    IndirectChannel channel_;

    bool moderationMode_{};
    bool isTopRightSplit_{};

    bool isMouseOver_{};
    bool isDragging_{};

    QVBoxLayout *const vbox_;
    SplitHeader *const header_;
    ChannelView *const view_;
    SplitInput *const input_;
    SplitOverlay *const overlay_;

    QPointer<OverlayWindow> overlayWindow_;

    QPointer<SelectChannelDialog> selectChannelDialog_;

    pajlada::Signals::Connection channelIDChangedConnection_;
    pajlada::Signals::Connection usermodeChangedConnection_;
    pajlada::Signals::Connection roomModeChangedConnection_;

    pajlada::Signals::Connection indirectChannelChangedConnection_;

    // This signal-holder is cleared whenever this split changes the underlying channel
    pajlada::Signals::SignalHolder channelSignalHolder_;

    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<boost::signals2::scoped_connection> bSignals_;

public Q_SLOTS:
    void addSibling();
    void deleteFromContainer();
    void changeChannel();
    void explainMoving();
    void explainSplitting();
    void popup();
    void showOverlayWindow();
    void clear();
    void openInBrowser();
    void openModViewInBrowser();
    void openWhispersInBrowser();
    void openBrowserPlayer();
    void openInStreamlink();
    void openWithCustomScheme();
    void setFiltersDialog();
    void showSearch(bool singleChannel);
    void showChatterList();
    void openSubPage();
    void reloadChannelAndSubscriberEmotes();
    void reconnect();
};

}  // namespace chatterino

QDebug operator<<(QDebug dbg, const chatterino::Split &split);
QDebug operator<<(QDebug dbg, const chatterino::Split *split);
