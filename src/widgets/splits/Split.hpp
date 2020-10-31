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
class Split : public BaseWidget, pajlada::Signals::SignalHolder
{
    friend class SplitInput;

    Q_OBJECT

public:
    explicit Split(SplitContainer *parent);
    explicit Split(QWidget *parent);

    ~Split() override;

    pajlada::Signals::NoArgSignal channelChanged;
    pajlada::Signals::NoArgSignal focused;
    pajlada::Signals::NoArgSignal focusLost;

    ChannelView &getChannelView();
    SplitContainer *getContainer();

    IndirectChannel getIndirectChannel();
    ChannelPtr getChannel();
    void setChannel(IndirectChannel newChannel);

    void setFilters(const QList<QUuid> ids);
    const QList<QUuid> getFilters() const;

    void setModerationMode(bool value);
    bool getModerationMode() const;

    void insertTextToInput(const QString &text);

    void showChangeChannelPopup(const char *dialogTitle, bool empty,
                                std::function<void(bool)> callback);
    void giveFocus(Qt::FocusReason reason);
    bool hasFocus() const;
    void updateGifEmotes();
    void updateLastReadMessage();
    void setIsTopRightSplit(bool value);

    void drag();

    bool isInContainer() const;

    void setContainer(SplitContainer *container);

    static pajlada::Signals::Signal<Qt::KeyboardModifiers>
        modifierStatusChanged;
    static Qt::KeyboardModifiers modifierStatus;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void channelNameUpdated(const QString &newChannelName);
    void handleModifiers(Qt::KeyboardModifiers modifiers);
    void onAccountSelected();

    SplitContainer *container_;
    IndirectChannel channel_;

    bool moderationMode_{};
    bool isTopRightSplit_{};

    bool isMouseOver_{};
    bool isDragging_{};

    QVBoxLayout *vbox_;
    SplitHeader *header_;
    ChannelView *view_;
    SplitInput *input_;
    SplitOverlay *overlay_;

    NullablePtr<SelectChannelDialog> selectChannelDialog_;

    pajlada::Signals::Connection channelIDChangedConnection_;
    pajlada::Signals::Connection usermodeChangedConnection_;
    pajlada::Signals::Connection roomModeChangedConnection_;

    pajlada::Signals::Connection indirectChannelChangedConnection_;
    pajlada::Signals::SignalHolder signalHolder_;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;

public slots:
    void addSibling();
    void deleteFromContainer();
    void changeChannel();
    void explainMoving();
    void explainSplitting();
    void popup();
    void clear();
    void openInBrowser();
    void openWhispersInBrowser();
    void openBrowserPlayer();
    void openInStreamlink();
    void openWithCustomScheme();
    void copyToClipboard();
    void setFiltersDialog();
    void showSearch();
    void showViewerList();
    void openSubPage();
    void reloadChannelAndSubscriberEmotes();
};

}  // namespace chatterino
