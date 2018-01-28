#pragma once

#include "logging/loggingchannel.hpp"
#include "messages/image.hpp"
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
struct Message;
}

class Channel : public std::enable_shared_from_this<Channel>
{
public:
    explicit Channel(const QString &_name);

    boost::signals2::signal<void(messages::MessagePtr &)> messageRemovedFromStart;
    boost::signals2::signal<void(messages::MessagePtr &)> messageAppended;
    boost::signals2::signal<void(std::vector<messages::MessagePtr> &)> messagesAddedAtStart;
    boost::signals2::signal<void(size_t index, messages::MessagePtr &)> messageReplaced;

    virtual bool isEmpty() const;
    messages::LimitedQueueSnapshot<messages::MessagePtr> getMessageSnapshot();

    void addMessage(messages::MessagePtr message);
    void addMessagesAtStart(std::vector<messages::MessagePtr> &messages);
    void replaceMessage(messages::MessagePtr message, messages::MessagePtr replacement);
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
    virtual bool isMod() const
    {
        return false;
    }

private:
    messages::LimitedQueue<messages::MessagePtr> messages;

    // std::shared_ptr<logging::Channel> loggingChannel;
};

typedef std::shared_ptr<Channel> SharedChannel;

}  // namespace chatterino
