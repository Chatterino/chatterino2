#pragma once

#include "messages/image.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "util/completionmodel.hpp"
#include "util/concurrentmap.hpp"

#include <QString>
#include <QTimer>
#include <boost/signals2.hpp>

#include <memory>

namespace chatterino {
namespace messages {
struct Message;
}

class Channel : public std::enable_shared_from_this<Channel>
{
    QTimer *clearCompletionModelTimer;

public:
    explicit Channel(const QString &_name);
    virtual ~Channel();

    pajlada::Signals::Signal<const QString &, const QString &> sendMessageSignal;

    boost::signals2::signal<void(messages::MessagePtr &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::MessagePtr &)> messageAppended;
    boost::signals2::signal<void(std::vector<messages::MessagePtr> &)> messagesAddedAtStart;
    boost::signals2::signal<void(size_t index, messages::MessagePtr &)> messageReplaced;
    pajlada::Signals::NoArgSignal destroyed;

    virtual bool isEmpty() const;
    messages::LimitedQueueSnapshot<messages::MessagePtr> getMessageSnapshot();

    void addMessage(messages::MessagePtr message);
    void addMessagesAtStart(std::vector<messages::MessagePtr> &messages);
    void replaceMessage(messages::MessagePtr message, messages::MessagePtr replacement);
    virtual void addRecentChatter(const std::shared_ptr<messages::Message> &message);

    QString name;
    QStringList modList;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const
    {
        return false;
    }

    static std::shared_ptr<Channel> getEmpty();

    CompletionModel completionModel;

protected:
    virtual void onConnected();

private:
    messages::LimitedQueue<messages::MessagePtr> messages;
};

using ChannelPtr = std::shared_ptr<Channel>;

}  // namespace chatterino
