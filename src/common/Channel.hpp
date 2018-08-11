#pragma once

#include "common/CompletionModel.hpp"
#include "messages/LimitedQueue.hpp"

#include <QString>
#include <QTimer>
#include <pajlada/signals/signal.hpp>

#include <memory>

namespace chatterino {

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

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
        Misc
    };

    explicit Channel(const QString &name, Type type);
    virtual ~Channel();

    pajlada::Signals::Signal<const QString &, const QString &, bool &>
        sendMessageSignal;

    pajlada::Signals::Signal<MessagePtr &> messageRemovedFromStart;
    pajlada::Signals::Signal<MessagePtr &> messageAppended;
    pajlada::Signals::Signal<std::vector<MessagePtr> &> messagesAddedAtStart;
    pajlada::Signals::Signal<size_t, MessagePtr &> messageReplaced;
    pajlada::Signals::NoArgSignal destroyed;

    Type getType() const;
    const QString &getName() const;
    bool isTwitchChannel() const;
    virtual bool isEmpty() const;
    LimitedQueueSnapshot<MessagePtr> getMessageSnapshot();

    void addMessage(MessagePtr message);
    void addMessagesAtStart(std::vector<MessagePtr> &messages_);
    void addOrReplaceTimeout(MessagePtr message);
    void disableAllMessages();
    void replaceMessage(MessagePtr message, MessagePtr replacement);
    virtual void addRecentChatter(const MessagePtr &message);

    QStringList modList;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;
    virtual bool isBroadcaster() const;
    virtual bool hasModRights() const;

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
