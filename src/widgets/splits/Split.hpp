#pragma once

#include "common/Aliases.hpp"
#include "common/Channel.hpp"
#include "common/NullablePtr.hpp"
#include "pajlada/signals/signalholder.hpp"
#include "widgets/BaseWidget.hpp"

#include <QFont>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/signals2.hpp>

namespace chatterino {

class ChannelView;
class SplitHeader;
class SplitInput;
class SplitContainer;
class SplitOverlay;
class SelectChannelDialog;

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

    // args: (SplitContainer::Direction dir, Split* parent)
    pajlada::Signals::Signal<int, Split *> insertSplitRequested;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
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
    void openChannelInStreamlink(QString channelName);
    /**
     * @brief Opens Twitch channel chat in a new Chatterino tab
     */
    void joinChannelInNewTab(ChannelPtr channel);

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

    NullablePtr<SelectChannelDialog> selectChannelDialog_;

    pajlada::Signals::Connection channelIDChangedConnection_;
    pajlada::Signals::Connection usermodeChangedConnection_;
    pajlada::Signals::Connection roomModeChangedConnection_;

    pajlada::Signals::Connection indirectChannelChangedConnection_;
    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<boost::signals2::scoped_connection> bSignals_;

public slots:
    void addSibling();
    void deleteFromContainer();
    void changeChannel();
    void explainMoving();
    void explainSplitting();
    void popup();
    void clear();
    void openInBrowser();
    void openModViewInBrowser();
    void openWhispersInBrowser();
    void openBrowserPlayer();
    void openInStreamlink();
    void openWithCustomScheme();
    void copyToClipboard();
    void startWatching();
    void setFiltersDialog();
    void showSearch(bool singleChannel);
    void showViewerList();
    void openSubPage();
    void reloadChannelAndSubscriberEmotes();
    void reconnect();
};

}  // namespace chatterino

QDebug operator<<(QDebug dbg, const chatterino::Split &split);
QDebug operator<<(QDebug dbg, const chatterino::Split *split);
