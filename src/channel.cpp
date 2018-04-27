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

Channel::Channel(const QString &_name, Type _type)
    : type(_type)
    , name(_name)
    , completionModel(this->name)
{
    this->clearCompletionModelTimer = new QTimer;
    QObject::connect(this->clearCompletionModelTimer, &QTimer::timeout, [this]() {
        this->completionModel.ClearExpiredStrings();  //
    });
    this->clearCompletionModelTimer->start(60 * 1000);
}

Channel::~Channel()
{
    this->destroyed.invoke();

    this->clearCompletionModelTimer->stop();
    this->clearCompletionModelTimer->deleteLater();
}

Channel::Type Channel::getType() const
{
    return this->type;
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

    bool isTimeout = (message->flags & Message::Timeout) != 0;

    if (!isTimeout) {
        const QString &username = message->loginName;
        if (!username.isEmpty()) {
            // TODO: Add recent chatters display name. This should maybe be a setting
            this->addRecentChatter(message);
        }
    }

    singletons::LoggingManager::getInstance().addMessage(this->name, message);

    if (isTimeout) {
        LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
        bool addMessage = true;
        int snapshotLength = snapshot.getLength();

        int end = std::max(0, snapshotLength - 20);

        for (int i = snapshotLength - 1; i >= end; --i) {
            auto &s = snapshot[i];
            if (s->flags.HasFlag(Message::Untimeout) && s->timeoutUser == message->timeoutUser) {
                break;
            }

            if (s->flags.HasFlag(Message::Timeout) && s->timeoutUser == message->timeoutUser) {
                assert(message->banAction != nullptr);
                MessagePtr replacement(
                    Message::createTimeoutMessage(*(message->banAction), s->count + 1));
                this->replaceMessage(s, replacement);
                addMessage = false;
            }
        }

        // disable the messages from the user
        for (int i = 0; i < snapshotLength; i++) {
            auto &s = snapshot[i];
            if ((s->flags & (Message::Timeout | Message::Untimeout)) == 0 &&
                s->loginName == message->timeoutUser) {
                s->flags.EnableFlag(Message::Disabled);
            }
        }

        // XXX: Might need the following line
        // WindowManager::getInstance().repaintVisibleChatWidgets(this);

        if (!addMessage) {
            return;
        }
    }

    if (this->messages.pushBack(message, deleted)) {
        this->messageRemovedFromStart.invoke(deleted);
    }

    this->messageAppended.invoke(message);
}

void Channel::addMessagesAtStart(std::vector<messages::MessagePtr> &_messages)
{
    std::vector<messages::MessagePtr> addedMessages = this->messages.pushFront(_messages);

    if (addedMessages.size() != 0) {
        this->messagesAddedAtStart.invoke(addedMessages);
    }
}

void Channel::replaceMessage(messages::MessagePtr message, messages::MessagePtr replacement)
{
    int index = this->messages.replaceItem(message, replacement);

    if (index >= 0) {
        this->messageReplaced.invoke((size_t)index, replacement);
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

bool Channel::isMod() const
{
    return false;
}

std::shared_ptr<Channel> Channel::getEmpty()
{
    static std::shared_ptr<Channel> channel(new Channel("", None));
    return channel;
}

void Channel::onConnected()
{
}

}  // namespace chatterino
