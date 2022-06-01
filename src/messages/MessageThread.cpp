#include "MessageThread.hpp"

#include "messages/Message.hpp"
#include "util/DebugCount.hpp"

namespace chatterino {

MessageThread::MessageThread(const std::shared_ptr<const Message> &rootMessage)
    : rootMessageId_(rootMessage->id)
    , rootMessage_(rootMessage)
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

}  // namespace chatterino
