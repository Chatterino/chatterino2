#pragma once

#include "channel.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageref.hpp"
#include "messages/word.hpp"
#include "messages/wordpart.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/channelview.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/helper/splitheader.hpp"
#include "widgets/helper/splitinput.hpp"

#include <QFont>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2/connection.hpp>

namespace chatterino {

class ChannelManager;
class ColorScheme;

namespace widgets {

class SplitContainer;

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
class Split : public BaseWidget
{
    friend class SplitInput;

    Q_OBJECT

public:
    Split(ChannelManager &_channelManager, SplitContainer *parent);
    ~Split();

    ChannelManager &channelManager;
    pajlada::Settings::Setting<std::string> channelName;
    boost::signals2::signal<void()> channelChanged;

    std::shared_ptr<Channel> getChannel() const;
    std::shared_ptr<Channel> &getChannelRef();
    void setFlexSizeX(double x);
    double getFlexSizeX();
    void setFlexSizeY(double y);
    double getFlexSizeY();

    bool showChangeChannelPopup(const char *dialogTitle, bool empty = false);
    void giveFocus(Qt::FocusReason reason);
    bool hasFocus() const;
    void layoutMessages();
    void updateGifEmotes();
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();

protected:
    virtual void paintEvent(QPaintEvent *) override;

private:
    SplitContainer &parentPage;
    std::shared_ptr<Channel> channel;

    QVBoxLayout vbox;
    SplitHeader header;
    ChannelView view;
    SplitInput input;
    double flexSizeX;
    double flexSizeY;

    boost::signals2::connection channelIDChangedConnection;

    void setChannel(std::shared_ptr<Channel> newChannel);
    void doOpenAccountPopupWidget(AccountPopupWidget *widget, QString user);
    void channelNameUpdated(const std::string &newChannelName);

public slots:
    // Add new split to the notebook page that this chat widget is in
    // Maybe we should use this chat widget as a hint to where the new split should be created
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

    // Open viewer list of the channel
    void doOpenViewerList();

    void doIncFlexX();
    void doDecFlexX();
    void doIncFlexY();
    void doDecFlexY();
};

}  // namespace widgets
}  // namespace chatterino
