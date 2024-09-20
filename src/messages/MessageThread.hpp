#pragma once

#include <boost/signals2.hpp>
#include <QString>

#include <memory>
#include <vector>

class QJsonObject;

namespace chatterino {
struct Message;

class MessageThread
{
public:
    enum class Subscription : uint8_t {
        None,
        Subscribed,
        Unsubscribed,
    };

    MessageThread(std::shared_ptr<const Message> rootMessage);
    ~MessageThread();

    void addToThread(const std::shared_ptr<const Message> &message);
    void addToThread(const std::weak_ptr<const Message> &message);

    /// Returns the number of live reply references
    size_t liveCount() const;

    /// Returns the number of live reply references
    size_t liveCount(const std::shared_ptr<const Message> &exclude) const;

    bool subscribed() const
    {
        return this->subscription_ == Subscription::Subscribed;
    }

    /// Returns true if and only if the user manually unsubscribed from the thread
    /// @see #markUnsubscribed()
    bool unsubscribed() const
    {
        return this->subscription_ == Subscription::Unsubscribed;
    }

    /// Subscribe to this thread.
    void markSubscribed();
    /// Unsubscribe from this thread.
    void markUnsubscribed();

    const QString &rootId() const
    {
        return rootMessageId_;
    }

    const std::shared_ptr<const Message> &root() const
    {
        return rootMessage_;
    }

    const std::vector<std::weak_ptr<const Message>> &replies() const
    {
        return replies_;
    }

    QJsonObject toJson() const;

    boost::signals2::signal<void()> subscriptionUpdated;

private:
    const QString rootMessageId_;
    const std::shared_ptr<const Message> rootMessage_;
    std::vector<std::weak_ptr<const Message>> replies_;

    Subscription subscription_ = Subscription::None;
};

}  // namespace chatterino
