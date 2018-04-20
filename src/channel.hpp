#pragma once

#include "messages/image.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "util/completionmodel.hpp"
#include "util/concurrentmap.hpp"

#include <QString>
#include <QTimer>
#include <pajlada/signals/signal.hpp>

#include <memory>

namespace chatterino {
namespace messages {
struct Message;
}  // namespace messages

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
    };

    explicit Channel(const QString &_name, Type type);
    virtual ~Channel();

    pajlada::Signals::Signal<const QString &, const QString &> sendMessageSignal;

    pajlada::Signals::Signal<messages::MessagePtr &> messageRemovedFromStart;
    pajlada::Signals::Signal<messages::MessagePtr &> messageAppended;
    pajlada::Signals::Signal<std::vector<messages::MessagePtr> &> messagesAddedAtStart;
    pajlada::Signals::Signal<size_t, messages::MessagePtr &> messageReplaced;
    pajlada::Signals::NoArgSignal destroyed;

    Type getType() const;
    virtual bool isEmpty() const;
    messages::LimitedQueueSnapshot<messages::MessagePtr> getMessageSnapshot();

    void addMessage(messages::MessagePtr message);
    void addMessagesAtStart(std::vector<messages::MessagePtr> &messages);
    void replaceMessage(messages::MessagePtr message, messages::MessagePtr replacement);
    virtual void addRecentChatter(const std::shared_ptr<messages::Message> &message);

    QString name;
    QStringList modList;

    virtual bool canSendMessage() const;
    virtual void sendMessage(const QString &message);
    virtual bool isMod() const;

    static std::shared_ptr<Channel> getEmpty();

    CompletionModel completionModel;

protected:
    virtual void onConnected();

private:
    messages::LimitedQueue<messages::MessagePtr> messages;
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
    IndirectChannel(ChannelPtr channel, Channel::Type type = Channel::None)
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
