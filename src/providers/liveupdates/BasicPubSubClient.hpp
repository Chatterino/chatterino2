#pragma once

#include "common/QLogging.hpp"
#include "providers/liveupdates/BasicPubSubWebsocket.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"

#include <pajlada/signals/signal.hpp>

#include <atomic>
#include <chrono>
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
template <typename Subscription>
class BasicPubSubClient
    : public std::enable_shared_from_this<BasicPubSubClient<Subscription>>
{
public:
    // The maximum amount of subscriptions this connections can handle
    const size_t maxSubscriptions;

    BasicPubSubClient(liveupdates::WebsocketClient &websocketClient,
                      liveupdates::WebsocketHandle handle,
                      size_t maxSubscriptions = 100)
        : maxSubscriptions(maxSubscriptions)
        , websocketClient_(websocketClient)
        , handle_(std::move(handle))
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

    bool send(const char *payload)
    {
        liveupdates::WebsocketErrorCode ec;
        this->websocketClient_.send(this->handle_, payload,
                                    websocketpp::frame::opcode::text, ec);

        if (ec)
        {
            qCDebug(chatterinoLiveupdates) << "Error sending message" << payload
                                           << ":" << ec.message().c_str();
            return false;
        }

        return true;
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
        this->send(encoded);

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
        this->send(encoded);

        return true;
    }

    void close(const std::string &reason,
               websocketpp::close::status::value code =
                   websocketpp::close::status::normal)
    {
        liveupdates::WebsocketErrorCode ec;

        auto conn = this->websocketClient_.get_con_from_hdl(this->handle_, ec);
        if (ec)
        {
            qCDebug(chatterinoLiveupdates)
                << "Error getting connection:" << ec.message().c_str();
            return;
        }

        conn->close(code, reason, ec);
        if (ec)
        {
            qCDebug(chatterinoLiveupdates)
                << "Error closing:" << ec.message().c_str();
            return;
        }
    }

    bool isStarted() const
    {
        return this->started_.load(std::memory_order_acquire);
    }

    /**
     * @brief Will be called when the clients has been requested to stop
     *
     * Derived classes can override this to implement their own shutdown behaviour
     */
    virtual void stopImpl()
    {
    }

    liveupdates::WebsocketClient &websocketClient_;

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

        this->stopImpl();
    }

    liveupdates::WebsocketHandle handle_;
    std::unordered_set<Subscription> subscriptions_;

    std::atomic<bool> started_{false};

    template <typename ManagerSubscription>
    friend class BasicPubSubManager;
};

}  // namespace chatterino
