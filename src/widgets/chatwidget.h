#pragma once

#include "channel.h"
#include "messages/limitedqueuesnapshot.h"
#include "messages/messageref.h"
#include "messages/word.h"
#include "messages/wordpart.h"
#include "widgets/chatwidgetheader.h"
#include "widgets/chatwidgetinput.h"
#include "widgets/chatwidgetview.h"

#include <QFont>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2/connection.hpp>

namespace chatterino {
namespace widgets {

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
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    SharedChannel getChannel() const;
    SharedChannel &getChannelRef();
    const QString &getChannelName() const;
    void setChannelName(const QString &name);

    void showChangeChannelPopup();
    messages::LimitedQueueSnapshot<messages::SharedMessageRef> getMessagesSnapshot();
    void layoutMessages();
    void updateGifEmotes();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void attachChannel(std::shared_ptr<Channel> _channel);
    void detachChannel();

    messages::LimitedQueue<messages::SharedMessageRef> _messages;

    SharedChannel _channel;
    QString _channelName;

    QFont _font;
    QVBoxLayout _vbox;
    ChatWidgetHeader _header;
    ChatWidgetView _view;
    ChatWidgetInput _input;

    boost::signals2::connection _messageAppendedConnection;
    boost::signals2::connection _messageRemovedConnection;

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
};

}  // namespace widgets
}  // namespace chatterino
