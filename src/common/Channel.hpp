#pragma once

#include "common/enums/MessageContext.hpp"
#include "controllers/completion/TabCompletionModel.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/MessageFlag.hpp"
#include "messages/MessageSink.hpp"

#include <magic_enum/magic_enum.hpp>
#include <pajlada/signals/signal.hpp>
#include <QDate>
#include <QString>
#include <QTimer>

#include <memory>
#include <optional>

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class TimeoutStackStyle : int {
    StackHard = 0,
    DontStackBeyondUserMessage = 1,
    DontStack = 2,

    Default = DontStackBeyondUserMessage,
};

class Channel : public std::enable_shared_from_this<Channel>, public MessageSink
{
public:
    // This is for Lua. See scripts/make_luals_meta.py
    /**
     * @exposeenum c2.ChannelType
     */
    enum class Type {
        None,
        Direct,
        Twitch,
        TwitchWhispers,
        TwitchWatching,
        TwitchMentions,
        TwitchLive,
        TwitchAutomod,
        TwitchEnd,
        Misc,
    };

    explicit Channel(const QString &name, Type type);
    ~Channel() override;

    // SIGNALS
    pajlada::Signals::Signal<const QString &, const QString &, bool &>
        sendMessageSignal;
    pajlada::Signals::Signal<const QString &, const QString &, const QString &,
                             bool &>
        sendReplySignal;
    pajlada::Signals::Signal<MessagePtr &, std::optional<MessageFlags>>
        messageAppended;
    pajlada::Signals::Signal<std::vector<MessagePtr> &> messagesAddedAtStart;
    /// (index, prev-message, replacement)
    pajlada::Signals::Signal<size_t, const MessagePtr &, const MessagePtr &>
        messageReplaced;
    /// Invoked when some number of messages were filled in using time received
    pajlada::Signals::Signal<const std::vector<MessagePtr> &> filledInMessages;
    pajlada::Signals::NoArgSignal destroyed;
    pajlada::Signals::NoArgSignal displayNameChanged;
    pajlada::Signals::NoArgSignal messagesCleared;

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
        MessagePtr message, MessageContext context,
        std::optional<MessageFlags> overridingFlags = std::nullopt) final;
    void addMessagesAtStart(const std::vector<MessagePtr> &messages_);

    void addSystemMessage(const QString &contents);

    /// Inserts the given messages in order by Message::serverReceivedTime.
    void fillInMissingMessages(const std::vector<MessagePtr> &messages);

    void addOrReplaceTimeout(MessagePtr message, const QDateTime &now) final;
    void addOrReplaceClearChat(MessagePtr message, const QDateTime &now) final;
    void disableAllMessages() final;
    void replaceMessage(const MessagePtr &message,
                        const MessagePtr &replacement);
    void replaceMessage(size_t index, const MessagePtr &replacement);
    void replaceMessage(size_t hint, const MessagePtr &message,
                        const MessagePtr &replacement);
    void disableMessage(const QString &messageID);

    /// Removes all messages from this channel and invokes #messagesCleared
    void clearMessages();

    MessagePtr findMessageByID(QStringView messageID) final;

    bool hasMessages() const;

    void applySimilarityFilters(const MessagePtr &message) const final;

    MessageSinkTraits sinkTraits() const final;

    // CHANNEL INFO
    virtual bool canSendMessage() const;
    virtual bool isWritable() const;  // whether split input will be usable
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;
    virtual bool isBroadcaster() const;
    virtual bool hasModRights() const;
    virtual bool hasHighRateLimit() const;
    virtual bool isLive() const;
    virtual bool isRerun() const;
    virtual bool shouldIgnoreHighlights() const;
    virtual bool canReconnect() const;
    virtual void reconnect();
    virtual QString getCurrentStreamID() const;

    static std::shared_ptr<Channel> getEmpty();

    TabCompletionModel *completionModel;
    QDate lastDate_;

protected:
    virtual void onConnected();
    virtual void messageRemovedFromStart(const MessagePtr &msg);
    QString platform_{"other"};

private:
    const QString name_;
    LimitedQueue<MessagePtr> messages_;
    Type type_;
    bool anythingLogged_ = false;
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

    ChannelPtr get() const;
    void reset(ChannelPtr channel);
    pajlada::Signals::NoArgSignal &getChannelChanged();
    Channel::Type getType();

private:
    std::shared_ptr<Data> data_;
};

}  // namespace chatterino
