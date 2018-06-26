#include "Channel.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "singletons/EmoteManager.hpp"
#include "singletons/IrcManager.hpp"
#include "singletons/LoggingManager.hpp"
#include "singletons/WindowManager.hpp"

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
    : name(_name)
    , completionModel(this->name)
    , type(_type)
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

bool Channel::isTwitchChannel() const
{
    return this->type >= Twitch && this->type < TwitchEnd;
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
    auto app = getApp();
    MessagePtr deleted;

    const QString &username = message->loginName;
    if (!username.isEmpty()) {
        // TODO: Add recent chatters display name. This should maybe be a setting
        this->addRecentChatter(message);
    }

    // FOURTF: change this when adding more providers
    if (this->isTwitchChannel()) {
        app->logging->addMessage(this->name, message);
    }

    if (this->messages.pushBack(message, deleted)) {
        this->messageRemovedFromStart.invoke(deleted);
    }

    this->messageAppended.invoke(message);
}

void Channel::addOrReplaceTimeout(messages::MessagePtr message)
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.getLength();

    int end = std::max(0, snapshotLength - 20);

    bool addMessage = true;

    QTime minimumTime = QTime::currentTime().addSecs(-5);

    for (int i = snapshotLength - 1; i >= end; --i) {
        auto &s = snapshot[i];

        qDebug() << s->parseTime << minimumTime;

        if (s->parseTime < minimumTime) {
            break;
        }

        if (s->flags.HasFlag(Message::Untimeout) && s->timeoutUser == message->timeoutUser) {
            break;
        }

        if (s->flags.HasFlag(Message::Timeout) && s->timeoutUser == message->timeoutUser) {
            if (message->flags.HasFlag(Message::PubSub) && !s->flags.HasFlag(Message::PubSub)) {
                this->replaceMessage(s, message);
                addMessage = false;
                break;
            }
            if (!message->flags.HasFlag(Message::PubSub) && s->flags.HasFlag(Message::PubSub)) {
                addMessage = false;
                break;
            }

            int count = s->count + 1;

            messages::MessagePtr replacement(Message::createSystemMessage(
                message->searchText + QString(" (") + QString::number(count) + " times)"));

            replacement->timeoutUser = message->timeoutUser;
            replacement->count = count;
            replacement->flags = message->flags;

            this->replaceMessage(s, replacement);

            return;
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

    if (addMessage) {
        this->addMessage(message);
    }

    // XXX: Might need the following line
    // WindowManager::getInstance().repaintVisibleChatWidgets(this);
}

void Channel::disableAllMessages()
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.getLength();
    for (int i = 0; i < snapshotLength; i++) {
        auto &s = snapshot[i];
        if (s->flags & Message::System || s->flags & Message::Timeout) {
            continue;
        }

        s->flags.EnableFlag(Message::Disabled);
    }
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

bool Channel::isBroadcaster() const
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
