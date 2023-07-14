#pragma once

#include "common/QLogging.hpp"
#include "providers/ws/Client.hpp"
#include "util/DebugCount.hpp"

#include <pajlada/signals/signal.hpp>

#include <atomic>
#include <chrono>
#include <unordered_set>
#include <utility>

namespace chatterino {

/**
 * This class manages a single connection
 * that has at most #maxSubscriptions subscriptions.
 *
 * You can safely overload the #onConnectionEstablished method
 * and e.g. add additional heartbeat logic.
 *
 * You can use shared_from_this to get a shared_ptr of this client.
 *
 * @tparam Subscription see BasicPubSubManager
 */
template <typename Subscription>
class BasicPubSubClient
    : public std::enable_shared_from_this<BasicPubSubClient<Subscription>>
{
public:
    // The maximum amount of subscriptions this connections can handle
    const size_t maxSubscriptions;

    BasicPubSubClient(ws::Client *client, ws::Connection conn,
                      size_t maxSubscriptions = 100)
        : maxSubscriptions(maxSubscriptions)
        , client_(client)
        , connection_(std::move(conn))
    {
    }

    virtual ~BasicPubSubClient() = default;

    BasicPubSubClient(const BasicPubSubClient &) = delete;
    BasicPubSubClient(const BasicPubSubClient &&) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &&) = delete;

protected:
    virtual void onConnectionEstablished()
    {
    }

    /**
     * @return true if this client subscribed to this subscription
     *         and the current subscriptions don't exceed the maximum
     *         amount.
     *         It won't subscribe twice to the same subscription.
     *         Don't use this in place of subscription management
     *         in the BasicPubSubManager.
     */
    bool subscribe(const Subscription &subscription)
    {
        if (this->subscriptions_.size() >= this->maxSubscriptions)
        {
            return false;
        }

        if (!this->subscriptions_.emplace(subscription).second)
        {
            qCWarning(chatterinoLiveupdates)
                << "Tried subscribing to" << subscription
                << "but we're already subscribed!";
            return true;  // true because the subscription already exists
        }

        qCDebug(chatterinoLiveupdates) << "Subscribing to" << subscription;
        DebugCount::increase("LiveUpdates subscriptions");

        QByteArray encoded = subscription.encodeSubscribe();
        this->client_->sendText(this->connection_, encoded);

        return true;
    }

    /**
     * @return true if this client previously subscribed
     *         and now unsubscribed from this subscription.
     */
    bool unsubscribe(const Subscription &subscription)
    {
        if (this->subscriptions_.erase(subscription) <= 0)
        {
            return false;
        }

        qCDebug(chatterinoLiveupdates) << "Unsubscribing from" << subscription;
        DebugCount::decrease("LiveUpdates subscriptions");

        QByteArray encoded = subscription.encodeUnsubscribe();
        this->client_->sendText(this->connection_, encoded);

        return true;
    }

    void close(const QString &reason,
               ws::Client::CloseCode code = ws::Client::CloseCode::Normal)
    {
        this->client_->close(this->connection_, reason, code);
    }

    bool isStarted() const
    {
        return this->started_.load(std::memory_order_acquire);
    }

    ws::Client *client_;

private:
    void start()
    {
        assert(!this->isStarted());
        this->started_.store(true, std::memory_order_release);
        this->onConnectionEstablished();
    }

    void stop()
    {
        assert(this->isStarted());
        this->started_.store(false, std::memory_order_release);
    }

    ws::Connection connection_;
    std::unordered_set<Subscription> subscriptions_;

    std::atomic<bool> started_{false};

    template <typename ManagerSubscription>
    friend class BasicPubSubManager;
};

}  // namespace chatterino
