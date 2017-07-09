#pragma once

#include "channel.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageref.hpp"
#include "messages/word.hpp"
#include "messages/wordpart.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/chatwidgetheader.hpp"
#include "widgets/chatwidgetinput.hpp"
#include "widgets/chatwidgetview.hpp"

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

class NotebookPage;

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
class ChatWidget : public BaseWidget
{
    Q_OBJECT

public:
    ChatWidget(ChannelManager &_channelManager, NotebookPage *parent);
    ~ChatWidget();

    std::shared_ptr<Channel> getChannel() const;
    std::shared_ptr<Channel> &getChannelRef();

    void showChangeChannelPopup();
    messages::LimitedQueueSnapshot<messages::SharedMessageRef> getMessagesSnapshot();
    void layoutMessages(bool forceUpdate = false);
    void updateGifEmotes();

    void giveFocus();

    pajlada::Settings::Setting<std::string> channelName;

protected:
    virtual void paintEvent(QPaintEvent *) override;

private:
    ChannelManager &channelManager;

    void setChannel(std::shared_ptr<Channel> newChannel);
    void detachChannel();

    void channelNameUpdated(const std::string &newChannelName);

    messages::LimitedQueue<messages::SharedMessageRef> messages;

    std::shared_ptr<Channel> channel;

    QVBoxLayout vbox;
    ChatWidgetHeader header;
    ChatWidgetView view;
    ChatWidgetInput input;

    boost::signals2::connection messageAppendedConnection;
    boost::signals2::connection messageRemovedConnection;

public:
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();

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
};

}  // namespace widgets
}  // namespace chatterino
