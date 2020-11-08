#include "common/Channel.hpp"

#include "Application.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/IrcMessageHandler.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace chatterino {

//
// Channel
//
Channel::Channel(const QString &name, Type type)
    : completionModel(*this)
    , name_(name)
    , type_(type)
{
}

Channel::~Channel()
{
    this->destroyed.invoke();
}

Channel::Type Channel::getType() const
{
    return this->type_;
}

const QString &Channel::getName() const
{
    return this->name_;
}

const QString &Channel::getDisplayName() const
{
    return this->getName();
}

bool Channel::isTwitchChannel() const
{
    return this->type_ >= Type::Twitch && this->type_ < Type::TwitchEnd;
}

bool Channel::isEmpty() const
{
    return this->name_.isEmpty();
}

bool Channel::hasMessages() const
{
    return !this->messages_.empty();
}

LimitedQueueSnapshot<MessagePtr> Channel::getMessageSnapshot()
{
    return this->messages_.getSnapshot();
}

void Channel::addMessage(MessagePtr message,
                         boost::optional<MessageFlags> overridingFlags)
{
    auto app = getApp();
    MessagePtr deleted;

    // FOURTF: change this when adding more providers
    if (this->isTwitchChannel() &&
        (!overridingFlags || !overridingFlags->has(MessageFlag::DoNotLog)))
    {
        app->logging->addMessage(this->name_, message);
    }

    if (this->messages_.pushBack(message, deleted))
    {
        this->messageRemovedFromStart.invoke(deleted);
    }

    this->messageAppended.invoke(message, overridingFlags);
}

void Channel::addOrReplaceTimeout(MessagePtr message)
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.size();

    int end = std::max(0, snapshotLength - 20);

    bool addMessage = true;

    QTime minimumTime = QTime::currentTime().addSecs(-5);

    auto timeoutStackStyle = static_cast<TimeoutStackStyle>(
        getSettings()->timeoutStackStyle.getValue());

    for (int i = snapshotLength - 1; i >= end; --i)
    {
        auto &s = snapshot[i];

        if (s->parseTime < minimumTime)
        {
            break;
        }

        if (s->flags.has(MessageFlag::Untimeout) &&
            s->timeoutUser == message->timeoutUser)
        {
            break;
        }

        if (timeoutStackStyle == TimeoutStackStyle::DontStackBeyondUserMessage)
        {
            if (s->loginName == message->timeoutUser &&
                s->flags.hasNone({MessageFlag::Disabled, MessageFlag::Timeout,
                                  MessageFlag::Untimeout}))
            {
                break;
            }
        }

        if (s->flags.has(MessageFlag::Timeout) &&
            s->timeoutUser == message->timeoutUser)
        {
            if (message->flags.has(MessageFlag::PubSub) &&
                !s->flags.has(MessageFlag::PubSub))
            {
                this->replaceMessage(s, message);
                addMessage = false;
                break;
            }
            if (!message->flags.has(MessageFlag::PubSub) &&
                s->flags.has(MessageFlag::PubSub))
            {
                addMessage = timeoutStackStyle == TimeoutStackStyle::DontStack;
                break;
            }

            int count = s->count + 1;

            MessageBuilder replacement(timeoutMessage, message->searchText,
                                       count);

            replacement->timeoutUser = message->timeoutUser;
            replacement->count = count;
            replacement->flags = message->flags;

            this->replaceMessage(s, replacement.release());

            addMessage = false;
            break;
        }
    }

    // disable the messages from the user
    for (int i = 0; i < snapshotLength; i++)
    {
        auto &s = snapshot[i];
        if (s->loginName == message->timeoutUser &&
            s->flags.hasNone({MessageFlag::Timeout, MessageFlag::Untimeout,
                              MessageFlag::Whisper}))
        {
            // FOURTF: disabled for now
            // PAJLADA: Shitty solution described in Message.hpp
            s->flags.set(MessageFlag::Disabled);
        }
    }

    if (addMessage)
    {
        this->addMessage(message);
    }

    // XXX: Might need the following line
    // WindowManager::instance().repaintVisibleChatWidgets(this);
}

void Channel::disableAllMessages()
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.size();
    for (int i = 0; i < snapshotLength; i++)
    {
        auto &message = snapshot[i];
        if (message->flags.hasAny({MessageFlag::System, MessageFlag::Timeout,
                                   MessageFlag::Whisper}))
        {
            continue;
        }

        // FOURTF: disabled for now
        const_cast<Message *>(message.get())->flags.set(MessageFlag::Disabled);
    }
}

void Channel::addMessagesAtStart(std::vector<MessagePtr> &_messages)
{
    std::vector<MessagePtr> addedMessages =
        this->messages_.pushFront(_messages);

    if (addedMessages.size() != 0)
    {
        this->messagesAddedAtStart.invoke(addedMessages);
    }
}

void Channel::replaceMessage(MessagePtr message, MessagePtr replacement)
{
    int index = this->messages_.replaceItem(message, replacement);

    if (index >= 0)
    {
        this->messageReplaced.invoke((size_t)index, replacement);
    }
}

void Channel::replaceMessage(size_t index, MessagePtr replacement)
{
    if (this->messages_.replaceItem(index, replacement))
    {
        this->messageReplaced.invoke(index, replacement);
    }
}

void Channel::deleteMessage(QString messageID)
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.size();

    int end = std::max(0, snapshotLength - 200);

    for (int i = snapshotLength - 1; i >= end; --i)
    {
        auto &s = snapshot[i];

        if (s->id == messageID)
        {
            s->flags.set(MessageFlag::Disabled);
            break;
        }
    }
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

bool Channel::hasModRights() const
{
    // fourtf: check if staff
    return this->isMod() || this->isBroadcaster();
}

bool Channel::hasHighRateLimit() const
{
    return this->isMod() || this->isBroadcaster();
}

bool Channel::isLive() const
{
    return false;
}

bool Channel::shouldIgnoreHighlights() const
{
    return this->type_ == Type::TwitchMentions ||
           this->type_ == Type::TwitchWhispers;
}

bool Channel::canReconnect() const
{
    return false;
}

void Channel::reconnect()
{
}

std::shared_ptr<Channel> Channel::getEmpty()
{
    static std::shared_ptr<Channel> channel(new Channel("", Type::None));
    return channel;
}

void Channel::onConnected()
{
}

//
// Indirect channel
//
IndirectChannel::Data::Data(ChannelPtr _channel, Channel::Type _type)
    : channel(_channel)
    , type(_type)
{
}

IndirectChannel::IndirectChannel(ChannelPtr channel, Channel::Type type)
    : data_(std::make_unique<Data>(channel, type))
{
}

ChannelPtr IndirectChannel::get()
{
    return data_->channel;
}

void IndirectChannel::reset(ChannelPtr channel)
{
    assert(this->data_->type != Channel::Type::Direct);

    this->data_->channel = channel;
    this->data_->changed.invoke();
}

pajlada::Signals::NoArgSignal &IndirectChannel::getChannelChanged()
{
    return this->data_->changed;
}

Channel::Type IndirectChannel::getType()
{
    if (this->data_->type == Channel::Type::Direct)
    {
        return this->get()->getType();
    }
    else
    {
        return this->data_->type;
    }
}

}  // namespace chatterino
