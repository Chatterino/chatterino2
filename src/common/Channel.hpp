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
    QTimer *clearCompletionModelTimer;

public:
    enum Type {
        None,
        Direct,
        Twitch,
        TwitchWhispers,
        TwitchWatching,
        TwitchMentions,
        TwitchEnd,
    };

    explicit Channel(const QString &_name, Type type);
    virtual ~Channel();

    pajlada::Signals::Signal<const QString &, const QString &, bool &> sendMessageSignal;

    pajlada::Signals::Signal<chatterino::MessagePtr &> messageRemovedFromStart;
    pajlada::Signals::Signal<chatterino::MessagePtr &> messageAppended;
    pajlada::Signals::Signal<std::vector<chatterino::MessagePtr> &> messagesAddedAtStart;
    pajlada::Signals::Signal<size_t, chatterino::MessagePtr &> messageReplaced;
    pajlada::Signals::NoArgSignal destroyed;

    Type getType() const;
    bool isTwitchChannel() const;
    virtual bool isEmpty() const;
    chatterino::LimitedQueueSnapshot<chatterino::MessagePtr> getMessageSnapshot();

    void addMessage(chatterino::MessagePtr message);
    void addMessagesAtStart(std::vector<chatterino::MessagePtr> &messages);
    void addOrReplaceTimeout(chatterino::MessagePtr message);
    void disableAllMessages();
    void replaceMessage(chatterino::MessagePtr message, chatterino::MessagePtr replacement);
    virtual void addRecentChatter(const std::shared_ptr<chatterino::Message> &message);

    QString name;
    QStringList modList;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;
    virtual bool isBroadcaster() const;

    static std::shared_ptr<Channel> getEmpty();

    CompletionModel completionModel;

protected:
    virtual void onConnected();

private:
    chatterino::LimitedQueue<chatterino::MessagePtr> messages;
    Type type;
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

    std::shared_ptr<Data> data;

public:
    IndirectChannel(ChannelPtr channel, Channel::Type type = Channel::Direct)
        : data(new Data(channel, type))
    {
    }

    ChannelPtr get()
    {
        return data->channel;
    }

    void update(ChannelPtr ptr)
    {
        assert(this->data->type != Channel::Direct);

        this->data->channel = ptr;
        this->data->changed.invoke();
    }

    pajlada::Signals::NoArgSignal &getChannelChanged()
    {
        return this->data->changed;
    }

    Channel::Type getType()
    {
        if (this->data->type == Channel::Direct) {
            return this->get()->getType();
        } else {
            return this->data->type;
        }
    }
};

}  // namespace chatterino
