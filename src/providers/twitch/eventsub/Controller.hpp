#pragma once

#include "providers/twitch/eventsub/SubscriptionHandle.hpp"
#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "twitch-eventsub-ws/session.hpp"
#include "util/ExponentialBackoff.hpp"
#include "util/ThreadGuard.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/functional/hash.hpp>
#include <QJsonObject>
#include <QString>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

namespace chatterino::eventsub {

class IController
{
public:
    virtual ~IController() = default;

    /// Removes one reference for the given subscription request
    ///
    /// Should realistically only be called in the dtor of SubscriptionHandle
    virtual void removeRef(const SubscriptionRequest &request) = 0;

    /// Mark the controller as quitting
    ///
    /// This lets us simplify some logic with unsubscriptions (i.e. we ignore it instead)
    virtual void setQuitting() = 0;

    /// Subscribe will make a request to each open connection and ask them to
    /// add this subscription.
    ///
    /// If this subscription already exists, this call is a no-op.
    ///
    /// If no open connection has room for this subscription, this function will
    /// create a new connection and queue up the subscription to run again after X seconds.
    [[nodiscard]] virtual SubscriptionHandle subscribe(
        const SubscriptionRequest &request) = 0;

    virtual void reconnectConnection(
        std::unique_ptr<lib::Listener> connection,
        const std::optional<std::string> &reconnectURL,
        const std::unordered_set<SubscriptionRequest> &subs) = 0;
};

class Controller : public IController
{
public:
    Controller();
    ~Controller() override;

    void removeRef(const SubscriptionRequest &request) override;

    void setQuitting() override;

    [[nodiscard]] SubscriptionHandle subscribe(
        const SubscriptionRequest &request) override;

    void reconnectConnection(
        std::unique_ptr<lib::Listener> connection,
        const std::optional<std::string> &reconnectURL,
        const std::unordered_set<SubscriptionRequest> &subs) override;

private:
    void subscribe(const SubscriptionRequest &request, bool isRetry);

    void createConnection();
    void createConnection(std::string host, std::string port, std::string path,
                          std::unique_ptr<lib::Listener> listener);
    void registerConnection(std::weak_ptr<lib::Session> &&connection);

    void retrySubscription(const SubscriptionRequest &request);

    void markRequestSubscribed(const SubscriptionRequest &request,
                               std::weak_ptr<lib::Session> connection,
                               const QString &subscriptionID);

    void markRequestFailed(const SubscriptionRequest &request);

    void markRequestUnsubscribed(const SubscriptionRequest &request);

    void clearConnections();

    const std::string userAgent;

    std::string eventSubHost;
    std::string eventSubPort;
    std::string eventSubPath;

    std::unique_ptr<std::thread> thread;
    std::unique_ptr<ThreadGuard> threadGuard;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::weak_ptr<lib::Session>> connections;

    [[nodiscard]] std::optional<std::shared_ptr<lib::Session>>
        getViableConnection(uint32_t &openButNotReadyConnections);

    struct Subscription {
        enum class State : uint8_t {
            /// No subscription attempt has been made, or we have unsubscribed after all references
            /// were released
            Unsubscribed,

            /// The subscription attempt failed, maxing out our retry attempts
            Failed,

            /// The initial subscription is currently in progress
            Subscribing,

            /// A retry is currently in progress
            Retrying,

            /// The subscription has been established
            Subscribed,

            /// We've lost interested in this subscription - currently unsubscribing
            Unsubscribing,
        } state = State::Unsubscribed;

        int32_t refCount = 0;
        std::weak_ptr<lib::Session> connection;

        /// The ID of the subscription the Twitch Helix API has given us
        QString subscriptionID;

        /// The timer, if any, for retrying the subscription creation
        std::unique_ptr<boost::asio::system_timer> retryTimer;
        // 500ms to 16s backoff
        ExponentialBackoff<6> backoff{std::chrono::milliseconds{500}};
    };

    std::mutex subscriptionsMutex;
    std::unordered_map<SubscriptionRequest, Subscription> subscriptions;

    std::atomic<bool> quitting = false;
};

class DummyController : public IController
{
public:
    ~DummyController() override = default;

    void removeRef(const SubscriptionRequest &request) override
    {
        (void)request;
    }

    void setQuitting() override
    {
        //
    }

    [[nodiscard]] SubscriptionHandle subscribe(
        const SubscriptionRequest &request) override
    {
        (void)request;
        return {};
    }

    void reconnectConnection(
        std::unique_ptr<lib::Listener> connection,
        const std::optional<std::string> &reconnectURL,
        const std::unordered_set<SubscriptionRequest> &subs) override;
};

}  // namespace chatterino::eventsub
