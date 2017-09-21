#pragma once

#include "concurrentmap.hpp"
#include "emotemanager.hpp"
#include "logging/loggingchannel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueue.hpp"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <boost/signals2.hpp>

#include <memory>

namespace chatterino {
namespace messages {
class Message;
}

class IrcManager;

class Channel
{
public:
    explicit Channel();

    boost::signals2::signal<void(messages::SharedMessage &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::SharedMessage &)> messageAppended;

    virtual bool isEmpty() const;
    messages::LimitedQueueSnapshot<messages::SharedMessage> getMessageSnapshot();

    // methods
    void addMessage(messages::SharedMessage message);
    QString name;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);

private:
    // variables
    messages::LimitedQueue<messages::SharedMessage> messages;

    // std::shared_ptr<logging::Channel> loggingChannel;
};

}  // namespace chatterino
