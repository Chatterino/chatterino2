#pragma once

#include <atomic>
#include <chrono>
#include <pajlada/signals/signal.hpp>
#include <unordered_set>

#include "common/QLogging.hpp"
#include "providers/liveupdates/BasicPubSubWebsocket.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

template <typename Subscription>
class BasicPubSubClient
    : public std::enable_shared_from_this<BasicPubSubClient<Subscription>>
{
public:
    // The max amount of subscriptions on a single connection
    static constexpr size_t MAX_SUBSCRIPTIONS = 100;

    BasicPubSubClient(liveupdates::WebsocketClient &websocketClient,
                      liveupdates::WebsocketHandle handle)
        : websocketClient_(websocketClient)
        , handle_(std::move(handle))
    {
    }

    virtual ~BasicPubSubClient() = default;

    BasicPubSubClient(const BasicPubSubClient &) = delete;
    BasicPubSubClient(const BasicPubSubClient &&) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &) = delete;
    BasicPubSubClient &operator=(const BasicPubSubClient &&) = delete;

    virtual void start()
    {
        assert(!this->isStarted());
        this->started_.store(true, std::memory_order_release);
    }

    void stop()
    {
        assert(this->isStarted());
        this->started_.store(false, std::memory_order_release);
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

    bool subscribe(const Subscription &subscription)
    {
        if (this->subscriptions_.size() >= BasicPubSubClient::MAX_SUBSCRIPTIONS)
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

    std::unordered_set<Subscription> getSubscriptions() const
    {
        return this->subscriptions_;
    }

protected:
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

    bool isStarted() const
    {
        return this->started_.load(std::memory_order_acquire);
    }

    liveupdates::WebsocketClient &websocketClient_;

private:
    liveupdates::WebsocketHandle handle_;
    std::unordered_set<Subscription> subscriptions_;

    std::atomic<bool> started_{false};
};

}  // namespace chatterino
