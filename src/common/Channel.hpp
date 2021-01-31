#pragma once

#include "common/CompletionModel.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/LimitedQueue.hpp"

#include <QString>
#include <QTimer>
#include <boost/optional.hpp>
#include <pajlada/signals/signal.hpp>

#include <memory>

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;
enum class MessageFlag : uint32_t;
using MessageFlags = FlagsEnum<MessageFlag>;

enum class TimeoutStackStyle : int {
    StackHard = 0,
    DontStackBeyondUserMessage = 1,
    DontStack = 2,

    Default = DontStackBeyondUserMessage,
};

class Channel : public std::enable_shared_from_this<Channel>
{
public:
    enum class Type {
        None,
        Direct,
        Twitch,
        TwitchWhispers,
        TwitchWatching,
        TwitchMentions,
        TwitchEnd,
        Irc,
        Misc
    };

    explicit Channel(const QString &name, Type type);
    virtual ~Channel();

    // SIGNALS
    pajlada::Signals::Signal<const QString &, const QString &, bool &>
        sendMessageSignal;
    pajlada::Signals::Signal<MessagePtr &> messageRemovedFromStart;
    pajlada::Signals::Signal<MessagePtr &, boost::optional<MessageFlags>>
        messageAppended;
    pajlada::Signals::Signal<std::vector<MessagePtr> &> messagesAddedAtStart;
    pajlada::Signals::Signal<size_t, MessagePtr &> messageReplaced;
    pajlada::Signals::NoArgSignal destroyed;
    pajlada::Signals::NoArgSignal displayNameChanged;

    Type getType() const;
    const QString &getName() const;
    virtual const QString &getDisplayName() const;
    virtual const QString &getLocalizedName() const;
    bool isTwitchChannel() const;
    virtual bool isEmpty() const;
    LimitedQueueSnapshot<MessagePtr> getMessageSnapshot();

    // MESSAGES
    // overridingFlags can be filled in with flags that should be used instead
    // of the message's flags. This is useful in case a flag is specific to a
    // type of split
    void addMessage(
        MessagePtr message,
        boost::optional<MessageFlags> overridingFlags = boost::none);
    void addMessagesAtStart(std::vector<MessagePtr> &messages_);
    void addOrReplaceTimeout(MessagePtr message);
    void disableAllMessages();
    void replaceMessage(MessagePtr message, MessagePtr replacement);
    void replaceMessage(size_t index, MessagePtr replacement);
    void deleteMessage(QString messageID);
    void clearMessages();

    bool hasMessages() const;

    QStringList modList;

    // CHANNEL INFO
    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;
    virtual bool isBroadcaster() const;
    virtual bool hasModRights() const;
    virtual bool hasHighRateLimit() const;
    virtual bool isLive() const;
    virtual bool shouldIgnoreHighlights() const;
    virtual bool canReconnect() const;
    virtual void reconnect();

    static std::shared_ptr<Channel> getEmpty();

    CompletionModel completionModel;

protected:
    virtual void onConnected();

private:
    const QString name_;
    LimitedQueue<MessagePtr> messages_;
    Type type_;
    QTimer clearCompletionModelTimer_;
};

using ChannelPtr = std::shared_ptr<Channel>;

class IndirectChannel
{
    struct Data {
        ChannelPtr channel;
        Channel::Type type;
        pajlada::Signals::NoArgSignal changed;

        Data(ChannelPtr channel, Channel::Type type);
    };

public:
    IndirectChannel(ChannelPtr channel,
                    Channel::Type type = Channel::Type::Direct);

    ChannelPtr get();
    void reset(ChannelPtr channel);
    pajlada::Signals::NoArgSignal &getChannelChanged();
    Channel::Type getType();

private:
    std::shared_ptr<Data> data_;
};

}  // namespace chatterino
