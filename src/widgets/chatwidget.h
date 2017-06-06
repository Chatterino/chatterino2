#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include "channel.h"
#include "messages/limitedqueuesnapshot.h"
#include "messages/messageref.h"
#include "messages/word.h"
#include "messages/wordpart.h"
#include "widgets/chatwidgetheader.h"
#include "widgets/chatwidgetinput.h"
#include "widgets/chatwidgetview.h"

#include <QFont>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2/connection.hpp>

namespace chatterino {
namespace widgets {

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(QWidget *parent = 0);
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
};

}  // namespace widgets
}  // namespace chatterino

#endif  // CHATWIDGET_H
