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

    ChatWidgetView &
    getView()
    {
        return view;
    }

    std::shared_ptr<Channel>
    getChannel() const
    {
        return channel;
    }

    const QString &
    getChannelName() const
    {
        return channelName;
    }

    void setChannelName(const QString &name);

    void showChangeChannelPopup();

    messages::LimitedQueueSnapshot<std::shared_ptr<messages::MessageRef>>
    getMessagesSnapshot()
    {
        return messages.getSnapshot();
    }

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    messages::LimitedQueue<std::shared_ptr<messages::MessageRef>> messages;

    std::shared_ptr<Channel> channel;
    QString channelName;

    QFont font;
    QVBoxLayout vbox;
    ChatWidgetHeader header;
    ChatWidgetView view;
    ChatWidgetInput input;

    boost::signals2::connection messageAppendedConnection;
    boost::signals2::connection messageRemovedConnection;

public:
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino

#endif  // CHATWIDGET_H
