#include "channel.hpp"
#include "debug/log.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "logging/loggingmanager.hpp"
#include "messages/message.hpp"
#include "windowmanager.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace chatterino::messages;

namespace chatterino {

Channel::Channel(const QString &_name)
    : name(_name)
//    , loggingChannel(logging::get(name))
{
}

bool Channel::isEmpty() const
{
    return false;
}

messages::LimitedQueueSnapshot<messages::SharedMessage> Channel::getMessageSnapshot()
{
    return this->messages.getSnapshot();
}

void Channel::addMessage(std::shared_ptr<Message> message)
{
    std::shared_ptr<Message> deleted;

    const QString &username = message->username;

    if (!username.isEmpty()) {
        // TODO: Add recent chatters display name. This should maybe be a setting
        this->addRecentChatter(username);
    }

    //    if (_loggingChannel.get() != nullptr) {
    //        _loggingChannel->append(message);
    //    }

    if (this->messages.appendItem(message, deleted)) {
        messageRemovedFromStart(deleted);
    }

    this->messageAppended(message);
}

void Channel::addRecentChatter(const QString &username)
{
    std::lock_guard<std::mutex> lock(this->recentChattersMutex);

    this->recentChatters.insert(username);
}

std::set<QString> Channel::getUsernamesForCompletions()
{
    std::set<QString> usernames;

    this->recentChattersMutex.lock();
    usernames.insert(this->recentChatters.begin(), this->recentChatters.end());
    this->recentChattersMutex.unlock();

    return usernames;
}

bool Channel::canSendMessage() const
{
    return false;
}

void Channel::sendMessage(const QString &message)
{
}

}  // namespace chatterino
