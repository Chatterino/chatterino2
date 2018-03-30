#include "channel.hpp"
#include "debug/log.hpp"
#include "messages/message.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/loggingmanager.hpp"
#include "singletons/windowmanager.hpp"

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
    , completionModel(this->name)
{
}

Channel::~Channel()
{
    this->destroyed.invoke();
}

bool Channel::isEmpty() const
{
    return this->name.isEmpty();
}

messages::LimitedQueueSnapshot<messages::MessagePtr> Channel::getMessageSnapshot()
{
    return this->messages.getSnapshot();
}

void Channel::addMessage(MessagePtr message)
{
    MessagePtr deleted;

    const QString &username = message->loginName;

    if (!username.isEmpty()) {
        // TODO: Add recent chatters display name. This should maybe be a setting
        this->addRecentChatter(message);
    }

    singletons::LoggingManager::getInstance().addMessage(this->name, message);

    if (this->messages.pushBack(message, deleted)) {
        this->messageRemovedFromStart(deleted);
    }

    this->messageAppended(message);
}

void Channel::addMessagesAtStart(std::vector<messages::MessagePtr> &_messages)
{
    std::vector<messages::MessagePtr> addedMessages = this->messages.pushFront(_messages);

    if (addedMessages.size() != 0) {
        this->messagesAddedAtStart(addedMessages);
    }
}

void Channel::replaceMessage(messages::MessagePtr message, messages::MessagePtr replacement)
{
    int index = this->messages.replaceItem(message, replacement);

    if (index >= 0) {
        this->messageReplaced((size_t)index, replacement);
    }
}

void Channel::addRecentChatter(const std::shared_ptr<messages::Message> &message)
{
    // Do nothing by default
}

bool Channel::canSendMessage() const
{
    return false;
}

void Channel::sendMessage(const QString &message)
{
}

std::shared_ptr<Channel> Channel::getEmpty()
{
    static std::shared_ptr<Channel> channel(new Channel(""));
    return channel;
}

void Channel::onConnected()
{
}

}  // namespace chatterino
