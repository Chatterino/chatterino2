#pragma once

#include "channel.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/layouts/messagelayoutelement.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageelement.hpp"
#include "util/serialize-custom.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/channelview.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/helper/splitheader.hpp"
#include "widgets/helper/splitinput.hpp"

#include <QFont>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {
namespace widgets {

class SplitContainer;
class SplitOverlay;

// Each ChatWidget consists of three sub-elements that handle their own part of the chat widget:
// ChatWidgetHeader
//   - Responsible for rendering which channel the ChatWidget is in, and the menu in the top-left of
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

    void setModerationMode(bool value);
    bool getModerationMode() const;

    void showChangeChannelPopup(const char *dialogTitle, bool empty,
                                std::function<void(bool)> callback);
    void giveFocus(Qt::FocusReason reason);
    bool hasFocus() const;
    void layoutMessages();
    void updateGifEmotes();
    void updateLastReadMessage();

    void drag();

    bool isInContainer() const;

    void setContainer(SplitContainer *container);

    static pajlada::Signals::Signal<Qt::KeyboardModifiers> modifierStatusChanged;
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

private:
    SplitContainer *container;
    IndirectChannel channel;

    QVBoxLayout vbox;
    SplitHeader header;
    ChannelView view;
    SplitInput input;
    SplitOverlay *overlay;

    bool moderationMode = false;

    bool isMouseOver = false;
    bool isDragging = false;

    pajlada::Signals::Connection channelIDChangedConnection;
    pajlada::Signals::Connection usermodeChangedConnection;
    pajlada::Signals::Connection roomModeChangedConnection;

    pajlada::Signals::Connection indirectChannelChangedConnection;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    void doOpenUserInfoPopup(const QString &user);
    void channelNameUpdated(const QString &newChannelName);
    void handleModifiers(Qt::KeyboardModifiers modifiers);

public slots:
    // Add new split to the notebook page that this chat widget is in
    // This is only activated from the menu now. Hotkey is handled in Notebook
    void doAddSplit();

    // Close current split (chat widget)
    void doCloseSplit();

    // Show a dialog for changing the current splits/chat widgets channel
    void doChangeChannel();

    // Open popup copy of this chat widget
    // XXX: maybe make current chatwidget a popup instead?
    void doPopup();

    // Clear chat from all messages
    void doClearChat();

    // Open link to twitch channel in default browser
    void doOpenChannel();

    // Open popup player of twitch channel in default browser
    void doOpenPopupPlayer();

    // Open twitch channel stream through streamlink
    void doOpenStreamlink();

    // Copy text from chat
    void doCopy();

    // Open a search popup
    void doSearch();

    // Open viewer list of the channel
    void doOpenViewerList();
};

}  // namespace widgets
}  // namespace chatterino
