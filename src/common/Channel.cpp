#include "common/Channel.hpp"

#include "Application.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageSimilarity.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Settings.hpp"
#include "util/ChannelHelpers.hpp"

namespace chatterino {

//
// Channel
//
Channel::Channel(const QString &name, Type type)
    : completionModel(new TabCompletionModel(*this, nullptr))
    , lastDate_(QDate::currentDate())
    , name_(name)
    , messages_(getSettings()->scrollbackSplitLimit)
    , type_(type)
{
    if (this->isTwitchChannel())
    {
        this->platform_ = "twitch";
    }
}

Channel::~Channel()
{
    auto *app = tryGetApp();
    if (app && this->anythingLogged_)
    {
        app->getChatLogger()->closeChannel(this->name_, this->platform_);
    }
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

const QString &Channel::getLocalizedName() const
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

void Channel::addMessage(MessagePtr message, MessageContext context,
                         std::optional<MessageFlags> overridingFlags)
{
    MessagePtr deleted;

    if (context == MessageContext::Original)
    {
        // Only log original messages
        auto isDoNotLogSet =
            (overridingFlags && overridingFlags->has(MessageFlag::DoNotLog)) ||
            message->flags.has(MessageFlag::DoNotLog);

        if (!isDoNotLogSet)
        {
            // Only log messages where the `DoNotLog` flag is not set
            getApp()->getChatLogger()->addMessage(this->name_, message,
                                                  this->platform_,
                                                  this->getCurrentStreamID());
            this->anythingLogged_ = true;
        }
    }

    if (this->messages_.pushBack(message, deleted))
    {
        this->messageRemovedFromStart(deleted);
    }

    this->messageAppended.invoke(message, overridingFlags);
}

void Channel::addSystemMessage(const QString &contents)
{
    auto msg = makeSystemMessage(contents);
    this->addMessage(msg, MessageContext::Original);
}

void Channel::addOrReplaceTimeout(MessagePtr message, const QDateTime &now)
{
    addOrReplaceChannelTimeout(
        this->getMessageSnapshot(), std::move(message), now,
        [this](auto /*idx*/, auto msg, auto replacement) {
            this->replaceMessage(msg, replacement);
        },
        [this](auto msg) {
            this->addMessage(msg, MessageContext::Original);
        },
        true);
}

void Channel::addOrReplaceClearChat(MessagePtr message, const QDateTime &now)
{
    addOrReplaceChannelClear(
        this->getMessageSnapshot(), std::move(message), now,
        [this](auto /*idx*/, auto msg, auto replacement) {
            this->replaceMessage(msg, replacement);
        },
        [this](auto msg) {
            this->addMessage(msg, MessageContext::Original);
        });
}

void Channel::disableAllMessages()
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.size();
    for (int i = 0; i < snapshotLength; i++)
    {
        const auto &message = snapshot[i];
        if (message->flags.hasAny({MessageFlag::System, MessageFlag::Timeout,
                                   MessageFlag::Whisper}))
        {
            continue;
        }

        // FOURTF: disabled for now
        const_cast<Message *>(message.get())->flags.set(MessageFlag::Disabled);
    }
}

void Channel::addMessagesAtStart(const std::vector<MessagePtr> &_messages)
{
    std::vector<MessagePtr> addedMessages =
        this->messages_.pushFront(_messages);

    if (addedMessages.size() != 0)
    {
        this->messagesAddedAtStart.invoke(addedMessages);
    }
}

void Channel::fillInMissingMessages(const std::vector<MessagePtr> &messages)
{
    if (messages.empty())
    {
        return;
    }

    auto snapshot = this->getMessageSnapshot();
    if (snapshot.size() == 0)
    {
        // There are no messages in this channel yet so we can just insert them
        // at the front in order
        this->messages_.pushFront(messages);
        this->filledInMessages.invoke(messages);
        return;
    }

    std::unordered_set<QString> existingMessageIds;
    existingMessageIds.reserve(snapshot.size());

    // First, collect the ids of every message already present in the channel
    for (const auto &msg : snapshot)
    {
        if (msg->flags.has(MessageFlag::System) || msg->id.isEmpty())
        {
            continue;
        }

        existingMessageIds.insert(msg->id);
    }

    bool anyInserted = false;

    // Keep track of the last message in the channel. We need this value
    // to allow concurrent appends to the end of the channel while still
    // being able to insert just-loaded historical messages at the end
    // in the correct place.
    auto lastMsg = snapshot[snapshot.size() - 1];
    for (const auto &msg : messages)
    {
        // check if message already exists
        if (existingMessageIds.count(msg->id) != 0)
        {
            continue;
        }

        // If we get to this point, we know we'll be inserting a message
        anyInserted = true;

        bool insertedFlag = false;
        for (const auto &snapshotMsg : snapshot)
        {
            if (snapshotMsg->flags.has(MessageFlag::System))
            {
                continue;
            }

            if (msg->serverReceivedTime < snapshotMsg->serverReceivedTime)
            {
                // We found the first message that comes after the current message.
                // Therefore, we can put the current message directly before. We
                // assume that the messages we are filling in are in ascending
                // order by serverReceivedTime.
                this->messages_.insertBefore(snapshotMsg, msg);
                insertedFlag = true;
                break;
            }
        }

        if (!insertedFlag)
        {
            // We never found a message already in the channel that came after
            // the current message. Put it at the end and make sure to update
            // which message is considered "the end".
            this->messages_.insertAfter(lastMsg, msg);
            lastMsg = msg;
        }
    }

    if (anyInserted)
    {
        // We only invoke a signal once at the end of filling all messages to
        // prevent doing any unnecessary repaints.
        this->filledInMessages.invoke(messages);
    }
}

void Channel::replaceMessage(const MessagePtr &message,
                             const MessagePtr &replacement)
{
    int index = this->messages_.replaceItem(message, replacement);

    if (index >= 0)
    {
        this->messageReplaced.invoke((size_t)index, message, replacement);
    }
}

void Channel::replaceMessage(size_t index, const MessagePtr &replacement)
{
    MessagePtr prev;
    if (this->messages_.replaceItem(index, replacement, &prev))
    {
        this->messageReplaced.invoke(index, prev, replacement);
    }
}

void Channel::replaceMessage(size_t hint, const MessagePtr &message,
                             const MessagePtr &replacement)
{
    auto index = this->messages_.replaceItem(hint, message, replacement);
    if (index >= 0)
    {
        this->messageReplaced.invoke(hint, message, replacement);
    }
}

void Channel::disableMessage(const QString &messageID)
{
    auto msg = this->findMessageByID(messageID);
    if (msg != nullptr)
    {
        msg->flags.set(MessageFlag::Disabled);
    }
}

void Channel::clearMessages()
{
    this->messages_.clear();
    this->messagesCleared.invoke();
}

MessagePtr Channel::findMessageByID(QStringView messageID)
{
    MessagePtr res;

    if (auto msg = this->messages_.rfind([messageID](const MessagePtr &msg) {
            return msg->id == messageID;
        });
        msg)
    {
        res = *msg;
    }

    return res;
}

void Channel::applySimilarityFilters(const MessagePtr &message) const
{
    setSimilarityFlags(message, this->messages_.getSnapshot());
}

MessageSinkTraits Channel::sinkTraits() const
{
    return {
        MessageSinkTrait::AddMentionsToGlobalChannel,
        MessageSinkTrait::RequiresKnownChannelPointReward,
    };
}

bool Channel::canSendMessage() const
{
    return false;
}

bool Channel::isWritable() const
{
    using Type = Channel::Type;
    auto type = this->getType();
    return type != Type::TwitchMentions && type != Type::TwitchLive &&
           type != Type::TwitchAutomod;
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

bool Channel::isRerun() const
{
    return false;
}

bool Channel::shouldIgnoreHighlights() const
{
    return this->type_ == Type::TwitchAutomod ||
           this->type_ == Type::TwitchMentions ||
           this->type_ == Type::TwitchWhispers;
}

bool Channel::canReconnect() const
{
    return false;
}

void Channel::reconnect()
{
}

QString Channel::getCurrentStreamID() const
{
    return {};
}

std::shared_ptr<Channel> Channel::getEmpty()
{
    static std::shared_ptr<Channel> channel(new Channel("", Type::None));
    return channel;
}

void Channel::onConnected()
{
}

void Channel::messageRemovedFromStart(const MessagePtr &msg)
{
}

//
// Indirect channel
//
IndirectChannel::Data::Data(ChannelPtr _channel, Channel::Type _type)
    : channel(std::move(_channel))
    , type(_type)
{
}

IndirectChannel::IndirectChannel(ChannelPtr channel, Channel::Type type)
    : data_(std::make_unique<Data>(std::move(channel), type))
{
}

ChannelPtr IndirectChannel::get() const
{
    return data_->channel;
}

void IndirectChannel::reset(ChannelPtr channel)
{
    assert(this->data_->type != Channel::Type::Direct);

    this->data_->channel = std::move(channel);
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
