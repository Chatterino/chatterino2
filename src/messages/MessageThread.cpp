#include "messages/MessageThread.hpp"

#include "common/Literals.hpp"
#include "messages/Message.hpp"
#include "util/DebugCount.hpp"
#include "util/QMagicEnum.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <utility>

namespace chatterino {

using namespace literals;

MessageThread::MessageThread(std::shared_ptr<const Message> rootMessage)
    : rootMessageId_(rootMessage->id)
    , rootMessage_(std::move(rootMessage))
{
    DebugCount::increase("message threads");
}

MessageThread::~MessageThread()
{
    DebugCount::decrease("message threads");
}

void MessageThread::addToThread(const std::shared_ptr<const Message> &message)
{
    this->replies_.emplace_back(message);
}

void MessageThread::addToThread(const std::weak_ptr<const Message> &message)
{
    this->replies_.push_back(message);
}

size_t MessageThread::liveCount() const
{
    size_t count = 0;
    for (auto reply : this->replies_)
    {
        if (!reply.expired())
        {
            ++count;
        }
    }

    return count;
}

size_t MessageThread::liveCount(
    const std::shared_ptr<const Message> &exclude) const
{
    size_t count = 0;
    for (auto reply : this->replies_)
    {
        if (!reply.expired() && reply.lock() != exclude)
        {
            ++count;
        }
    }

    return count;
}

void MessageThread::markSubscribed()
{
    if (this->subscription_ == Subscription::Subscribed)
    {
        return;
    }

    this->subscription_ = Subscription::Subscribed;
    this->subscriptionUpdated();
}

void MessageThread::markUnsubscribed()
{
    if (this->subscription_ == Subscription::Unsubscribed)
    {
        return;
    }

    this->subscription_ = Subscription::Unsubscribed;
    this->subscriptionUpdated();
}

QJsonObject MessageThread::toJson() const
{
    QJsonObject obj{
        {"rootId"_L1, this->rootMessageId_},
        {"subscription"_L1, qmagicenum::enumNameString(this->subscription_)},
    };

    QJsonArray replies;
    for (const auto &msg : this->replies_)
    {
        auto locked = msg.lock();
        if (locked)
        {
            replies.append(locked->id);
        }
        else
        {
            replies.append(QJsonValue::Null);
        }
    }
    obj["replies"_L1] = replies;

    return obj;
}

}  // namespace chatterino
