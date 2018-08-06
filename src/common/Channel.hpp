#pragma once

#include "common/CompletionModel.hpp"
#include "messages/Image.hpp"
#include "messages/LimitedQueue.hpp"
#include "messages/Message.hpp"
#include "util/ConcurrentMap.hpp"

#include <QString>
#include <QTimer>
#include <pajlada/signals/signal.hpp>

#include <memory>

namespace chatterino {
struct Message;

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

    pajlada::Signals::Signal<const QString &, const QString &, bool &> sendMessageSignal;

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
    virtual void addRecentChatter(const std::shared_ptr<Message> &message);

    QStringList modList;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;
    virtual bool isBroadcaster() const;
    virtual bool hasModRights() const;

    static std::shared_ptr<Channel> getEmpty();

    CompletionModel completionModel;

    // pre c++17 polyfill
    std::weak_ptr<Channel> weak_from_this();

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

        Data() = delete;
        Data(ChannelPtr _channel, Channel::Type _type)
            : channel(_channel)
            , type(_type)
        {
        }
    };

public:
    IndirectChannel(ChannelPtr channel, Channel::Type type = Channel::Type::Direct)
        : data_(new Data(channel, type))
    {
    }

    ChannelPtr get()
    {
        return data_->channel;
    }

    void update(ChannelPtr ptr)
    {
        assert(this->data_->type != Channel::Type::Direct);

        this->data_->channel = ptr;
        this->data_->changed.invoke();
    }

    pajlada::Signals::NoArgSignal &getChannelChanged()
    {
        return this->data_->changed;
    }

    Channel::Type getType()
    {
        if (this->data_->type == Channel::Type::Direct) {
            return this->get()->getType();
        } else {
            return this->data_->type;
        }
    }

private:
    std::shared_ptr<Data> data_;
};

}  // namespace chatterino
