#pragma once

#include "logging/loggingchannel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueue.hpp"
#include "util/concurrentmap.hpp"

#include <QMap>
#include <QMutex>
#include <QString>
#include <QVector>
#include <boost/signals2.hpp>

#include <memory>
#include <set>

namespace chatterino {
namespace messages {
class Message;
}

class Channel : public std::enable_shared_from_this<Channel>
{
public:
    explicit Channel(const QString &_name);

    boost::signals2::signal<void(messages::SharedMessage &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::SharedMessage &)> messageAppended;
    boost::signals2::signal<void(std::vector<messages::SharedMessage> &)> messagesAddedAtStart;

    virtual bool isEmpty() const;
    messages::LimitedQueueSnapshot<messages::SharedMessage> getMessageSnapshot();

    void addMessage(messages::SharedMessage message);
    void addMessagesAtStart(std::vector<messages::SharedMessage> &messages);
    void addRecentChatter(const std::shared_ptr<messages::Message> &message);

    struct NameOptions {
        QString displayName;
        QString localizedName;
    };

    std::vector<NameOptions> getUsernamesForCompletions();

    QString name;
    QStringList modList;

    // Key = login name
    std::map<QString, NameOptions> recentChatters;
    std::mutex recentChattersMutex;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);

private:
    messages::LimitedQueue<messages::SharedMessage> messages;

    // std::shared_ptr<logging::Channel> loggingChannel;
};

}  // namespace chatterino
