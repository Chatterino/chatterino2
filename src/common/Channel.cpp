#include "common/Channel.hpp"

#include "Application.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
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
    , lastDate_(QDate::currentDate())
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

void Channel::addMessage(MessagePtr message,
                         boost::optional<MessageFlags> overridingFlags)
{
    auto app = getApp();
    MessagePtr deleted;

    if (!overridingFlags || !overridingFlags->has(MessageFlag::DoNotLog))
    {
        QString channelPlatform("other");
        if (this->type_ == Type::Irc)
        {
            auto irc = static_cast<IrcChannel *>(this);
            channelPlatform =
                QString("irc-%1").arg(irc->server()->userFriendlyIdentifier());
        }
        else if (this->isTwitchChannel())
        {
            channelPlatform = "twitch";
        }
        app->logging->addMessage(this->name_, message, channelPlatform);
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

void Channel::addOrReplaceSevenTvEventAddRemove(MessagePtr message)
{
    LimitedQueueSnapshot<MessagePtr> snapshot = this->getMessageSnapshot();
    int snapshotLength = snapshot.size();

    int end = std::max(0, snapshotLength - 20);

    auto addMessage = true;
    auto skip = false;

    QTime minimumTime = QTime::currentTime().addSecs(-5);

    MessageFlag currentFlag;
    if (message->flags.has(MessageFlag::SevenTvEventApiAddEmoteMessage))
    {
        currentFlag = MessageFlag::SevenTvEventApiAddEmoteMessage;
    }
    else if (message->flags.has(MessageFlag::SevenTvEventApiRemoveEmoteMessage))
    {
        currentFlag = MessageFlag::SevenTvEventApiRemoveEmoteMessage;
    }
    else
    {
        // this case should be unreachable because this function is only called with messages which contain the above flags
        skip = true;
    }

    if (!skip)
    {
        for (int i = snapshotLength - 1; i >= end; --i)
        {
            auto &s = snapshot[i];

            if (s->parseTime < minimumTime)
            {
                break;
            }

            if (s->flags.has(currentFlag) && s->loginName != message->loginName)
                break;

            if (s->flags.hasAny(
                    {MessageFlag::SevenTvEventApiAddEmoteMessage,
                     MessageFlag::SevenTvEventApiRemoveEmoteMessage,
                     MessageFlag::SevenTvEventApiUpdateEmoteMessage}))
            {
                auto anyMatchingEmote = std::any_of(
                    s->seventvEventTargetEmotes.begin(),
                    s->seventvEventTargetEmotes.end(),
                    [messageEmotes =
                         message->seventvEventTargetEmotes](const auto &emote) {
                        return std::find(messageEmotes.begin(),
                                         messageEmotes.end(),
                                         emote) != messageEmotes.end();
                    });

                if (anyMatchingEmote)
                    break;
            }

            if (s->flags.has(currentFlag))
            {
                auto emotes = s->seventvEventTargetEmotes;
                for (auto const &e : message->seventvEventTargetEmotes)
                {
                    emotes.push_back(e);
                }

                MessageBuilder replacement;
                if (currentFlag == MessageFlag::SevenTvEventApiAddEmoteMessage)
                {
                    replacement = MessageBuilder(seventvAddEmoteMessage,
                                                 message->loginName, emotes);
                }
                else  // current == RemoveEmoteMessage
                {
                    replacement = MessageBuilder(seventvRemoveEmoteMessage,
                                                 message->loginName, emotes);
                }

                replacement->flags = message->flags;

                this->replaceMessage(s, replacement.release());

                addMessage = false;
                break;
            }
        }
    }

    if (addMessage)
    {
        this->addMessage(message);
    }
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
    auto snapshot = this->getMessageSnapshot();

    std::unordered_set<QString> existingMessageIds;
    existingMessageIds.reserve(snapshot.size());

    // First, collect the ids of every message already present in the channel
    for (auto &msg : snapshot)
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
    for (auto &msg : messages)
    {
        // check if message already exists
        if (existingMessageIds.count(msg->id) != 0)
        {
            continue;
        }

        // If we get to this point, we know we'll be inserting a message
        anyInserted = true;

        bool insertedFlag = false;
        for (auto &snapshotMsg : snapshot)
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
    auto msg = this->findMessage(messageID);
    if (msg != nullptr)
    {
        msg->flags.set(MessageFlag::Disabled);
    }
}

MessagePtr Channel::findMessage(QString messageID)
{
    MessagePtr res;

    if (auto msg = this->messages_.rfind([&messageID](const MessagePtr &msg) {
            return msg->id == messageID;
        });
        msg)
    {
        res = *msg;
    }

    return res;
}

bool Channel::canSendMessage() const
{
    return false;
}

bool Channel::isWritable() const
{
    using Type = Channel::Type;
    auto type = this->getType();
    return type != Type::TwitchMentions && type != Type::TwitchLive;
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
