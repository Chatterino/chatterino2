#pragma once

#include "providers/twitch/eventsub/SubscriptionHandle.hpp"
#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "twitch-eventsub-ws/session.hpp"
#include "util/ThreadGuard.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/functional/hash.hpp>
#include <QJsonObject>
#include <QString>

#include <memory>
#include <string>
#include <thread>

namespace chatterino::eventsub {

class IController
{
public:
    virtual ~IController() = default;

    virtual void removeRef(const SubscriptionRequest &request) = 0;

    /// Subscribe will make a request to each open connection and ask them to
    /// add this subscription.
    ///
    /// If this subscription already exists, this call is a no-op.
    ///
    /// If no open connection has room for this subscription, this function will
    /// create a new connection and queue up the subscription to run again after X seconds.
    [[nodiscard]] virtual SubscriptionHandle subscribe(
        const SubscriptionRequest &request) = 0;
};

class Controller : public IController
{
public:
    Controller();
    ~Controller() override;

    void removeRef(const SubscriptionRequest &request) override;

    [[nodiscard]] SubscriptionHandle subscribe(
        const SubscriptionRequest &request) override;

private:
    void subscribe(const SubscriptionRequest &request, bool isQueued);

    void createConnection();
    void registerConnection(std::weak_ptr<lib::Session> &&connection);

    void queueSubscription(const SubscriptionRequest &request,
                           boost::posix_time::time_duration delay);

    void markRequestSubscribed(const SubscriptionRequest &request,
                               std::weak_ptr<lib::Session> connection);

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

    struct XD {
        int32_t refCount = 0;
        std::weak_ptr<lib::Session> connection;
    };

    std::mutex subscriptionsMutex;
    std::unordered_map<SubscriptionRequest, XD> activeSubscriptions;

    std::unordered_map<SubscriptionRequest,
                       std::unique_ptr<boost::asio::deadline_timer>>
        queuedSubscriptions;
};

class DummyController : public IController
{
public:
    ~DummyController() override = default;

    void removeRef(const SubscriptionRequest &request) override
    {
        (void)request;
    };

    [[nodiscard]] SubscriptionHandle subscribe(
        const SubscriptionRequest &request) override
    {
        (void)request;
        return {};
    };
};

}  // namespace chatterino::eventsub
