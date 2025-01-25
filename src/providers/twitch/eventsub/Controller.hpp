#pragma once

#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/functional/hash.hpp>
#include <QJsonObject>
#include <QString>

#include <memory>
#include <string>
#include <thread>

namespace chatterino::eventsub {

class Controller
{
public:
    Controller();
    ~Controller();

    /// Subscribe will make a request to each open connection and ask them to
    /// add this subscription.
    ///
    /// If this subscription already exists, this call is a no-op.
    ///
    /// If no open connection has room for this subscription, this function will
    /// create a new connection and queue up the subscription to run again after X seconds.
    ///
    /// TODO: Return a SubscriptionHandle that handles unsubscriptions
    /// Dupe subscriptions should return shared subscription handles
    /// So no more owners of the subscription handle means we send an unsubscribe request
    void subscribe(const SubscriptionRequest &request, bool isQueued = false);

private:
    void createConnection();
    void registerConnection(std::weak_ptr<lib::Session> &&connection);

    const std::string userAgent;

    std::string eventSubHost;
    std::string eventSubPort;
    std::string eventSubPath;

    std::unique_ptr<std::thread> thread;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::weak_ptr<lib::Session>> connections;

    std::unordered_map<SubscriptionRequest,
                       std::unique_ptr<boost::asio::deadline_timer>>
        queuedSubscriptions;
};

}  // namespace chatterino::eventsub
