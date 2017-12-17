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

    const QString &username = message->loginName;

    if (!username.isEmpty()) {
        // TODO: Add recent chatters display name. This should maybe be a setting
        this->addRecentChatter(message);
    }

    //    if (_loggingChannel.get() != nullptr) {
    //        _loggingChannel->append(message);
    //    }

    if (this->messages.appendItem(message, deleted)) {
        messageRemovedFromStart(deleted);
    }

    this->messageAppended(message);
}

void Channel::addRecentChatter(const std::shared_ptr<messages::Message> &message)
{
    assert(!message->loginName.isEmpty());

    std::lock_guard<std::mutex> lock(this->recentChattersMutex);

    this->recentChatters[message->loginName] = {message->displayName, message->localizedName};
}

std::vector<Channel::NameOptions> Channel::getUsernamesForCompletions()
{
    std::vector<NameOptions> names;

    this->recentChattersMutex.lock();
    for (const auto &p : this->recentChatters) {
        names.push_back(p.second);
    }
    // usernames.insert(this->recentChatters.begin(), this->recentChatters.end());
    this->recentChattersMutex.unlock();

    return names;
}

bool Channel::canSendMessage() const
{
    return false;
}

void Channel::sendMessage(const QString &message)
{
}

}  // namespace chatterino
