#pragma once

#include "common/QLogging.hpp"
#include "common/websockets/WebSocketPool.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "util/DebugCount.hpp"

#include <unordered_set>

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
template <typename SubscriptionT, typename Derived>
class BasicPubSubClient
{
public:
    using Subscription = SubscriptionT;

    // The maximum amount of subscriptions this connections can handle
    const size_t maxSubscriptions;

    BasicPubSubClient(size_t maxSubscriptions = 100)
        : maxSubscriptions(maxSubscriptions)
    {
    }

    ~BasicPubSubClient() = default;
    BasicPubSubClient(const BasicPubSubClient &) = delete;
    BasicPubSubClient(const BasicPubSubClient &&) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &&) = delete;

    /// The websocket handshake completed.
    ///
    /// Called from the manager in the GUI thread.
    void onOpen()
    {
        assertInGuiThread();
        this->open_ = true;
    }

    /// A message has been received.
    ///
    /// Called from the websocket thread.
    void onMessage(const QByteArray & /*msg*/)
    {
    }

    void close()
    {
        this->ws_.close();
    }

    void sendText(const QByteArray &data)
    {
        this->ws_.sendText(data);
    }

    QByteArray encodeSubscription(const Subscription &subscription)
    {
        return subscription.encodeSubscribe();
    }

    QByteArray encodeUnsubscription(const Subscription &subscription)
    {
        return subscription.encodeUnsubscribe();
    }

protected:
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

        QByteArray encoded =
            static_cast<Derived *>(this)->encodeSubscription(subscription);
        this->ws_.sendText(encoded);

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

        QByteArray encoded =
            static_cast<Derived *>(this)->encodeUnsubscription(subscription);
        this->ws_.sendText(encoded);

        return true;
    }

    bool isOpen() const
    {
        return this->open_;
    }

private:
    WebSocketHandle ws_;
    std::unordered_set<Subscription> subscriptions_;

    bool open_ = false;

    template <typename ManagerSubscription, typename ManagerClient>
    friend class BasicPubSubManager;
};

}  // namespace chatterino
