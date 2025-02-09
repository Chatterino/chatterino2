#include "providers/twitch/eventsub/Controller.hpp"

#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/eventsub/Connection.hpp"
#include "util/RenameThread.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/certify/https_verification.hpp>
#include <twitch-eventsub-ws/session.hpp>

#include <memory>
#include <utility>

namespace {

/// Enable LOCAL_EVENTSUB when you want to debug eventsub with a local instance of the Twitch CLI
/// twitch event websocket start-server --ssl --port 3012
constexpr bool LOCAL_EVENTSUB = false;

std::tuple<std::string, std::string, std::string> getEventSubHost()
{
    if constexpr (LOCAL_EVENTSUB)
    {
        return {"localhost", "3012", "/ws"};
    }

    return {"eventsub.wss.twitch.tv", "443", "/ws"};
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoTwitchEventSub;

}  // namespace

namespace chatterino::eventsub {

Controller::Controller()
    : userAgent(QStringLiteral("chatterino/%1 (%2)")
                    .arg(Version::instance().version(),
                         Version::instance().commitHash())
                    .toUtf8()
                    .toStdString())
    , ioContext(1)
    , work(boost::asio::make_work_guard(this->ioContext))
{
    std::tie(this->eventSubHost, this->eventSubPort, this->eventSubPath) =
        getEventSubHost();
    this->thread = std::make_unique<std::thread>([this] {
        this->ioContext.run();
    });
    renameThread(*this->thread, "C2EventSub");

    this->threadGuard = std::make_unique<ThreadGuard>(this->thread->get_id());
}

Controller::~Controller()
{
    qCInfo(LOG) << "Controller dtor start";

    for (const auto &weakConnection : this->connections)
    {
        auto connection = weakConnection.lock();
        if (!connection)
        {
            continue;
        }

        connection->close();
    }

    this->subscriptions.clear();

    this->work.reset();

    if (this->thread->joinable())
    {
        this->thread->join();
    }
    else
    {
        qCWarning(LOG) << "Thread not joinable";
    }

    qCInfo(LOG) << "Controller dtor end";
}

void Controller::removeRef(const SubscriptionRequest &request)
{
    std::lock_guard lock(this->subscriptionsMutex);

    assert(this->subscriptions.contains(request));

    auto &subscription = this->subscriptions[request];
    subscription.refCount--;
    qCInfo(LOG) << "Removed ref for" << request << subscription.refCount;

    if (subscription.refCount <= 0)
    {
        if (subscription.subscriptionID.isEmpty())
        {
            qCWarning(LOG) << "Refcount fell to zero for" << request
                           << "but we had no subscription ID attached - was a "
                              "successful subscription never made?";
            return;
        }

        qCInfo(LOG) << "Unsubscribing from" << request;
        getHelix()->deleteEventSubSubscription(
            subscription.subscriptionID,
            [request] {
                qCInfo(LOG) << "Successfully unsubscribed from" << request;
            },
            [request](const auto &errorMessage) {
                qCInfo(LOG)
                    << "An error occurred while attempting to unsubscribe from"
                    << request << errorMessage;
            });

        subscription.subscriptionID.clear();
    }
}

SubscriptionHandle Controller::subscribe(const SubscriptionRequest &request)
{
    bool needToSubscribe = false;

    {
        std::lock_guard lock(this->subscriptionsMutex);

        auto &subscription = this->subscriptions[request];
        if (subscription.refCount == 0)
        {
            needToSubscribe = true;
        }
        subscription.refCount++;
        qCInfo(LOG) << "Added ref for" << request << subscription.refCount;
    }

    auto handle = std::make_unique<RawSubscriptionHandle>(request);

    if (needToSubscribe)
    {
        boost::asio::post(this->ioContext, [this, request] {
            this->subscribe(request, false);
        });
    }

    return handle;
}

void Controller::subscribe(const SubscriptionRequest &request, bool isRetry)
{
    qCInfo(LOG) << "Subscribe request for" << request.subscriptionType;
    boost::asio::post(this->ioContext, [this, request, isRetry] {
        // 1. Flush dead connections (maybe this should not be done here)
        // TODO: implement

        {
            std::lock_guard lock(this->subscriptionsMutex);
            auto &subscription = this->subscriptions[request];
            if (isRetry)
            {
                qCInfo(LOG) << "Removing subscription from queued list";

                subscription.retryTimer.reset();
            }
            else if (subscription.retryTimer)
            {
                qCWarning(LOG)
                    << "We already have a queued subscription for this, "
                       "let's chill :)";
                return;
            }
        }

        uint32_t openButNotReadyConnections = 0;

        // 2. Check if any currently open connection can handle this subscription
        for (const auto &weakConnection : this->connections)
        {
            auto connection = weakConnection.lock();
            if (!connection)
            {
                // TODO: remove it here?
                continue;
            }

            auto *listener =
                dynamic_cast<Connection *>(connection->getListener());

            if (listener == nullptr)
            {
                // something really goofy is going on
                qCWarning(LOG) << "listener was not the correct type";
                continue;
            }

            if (listener->getSessionID().isEmpty())
            {
                // This connection is open but it's not ready (i.e. no welcome has been posted yet)
                ++openButNotReadyConnections;
                continue;
            }

            // TODO: Check if this listener has room for another subscription

            // TODO: Don't hardcode the subscription version
            getHelix()->createEventSubSubscription(
                request, listener->getSessionID(),
                [this, request, connection,
                 weakConnection{weakConnection}](const auto &res) {
                    qCInfo(LOG) << "success" << res;
                    this->markRequestSubscribed(request, weakConnection,
                                                res.subscriptionID);
                },
                [this, request](const auto &error, const auto &errorString) {
                    using Error = HelixCreateEventSubSubscriptionError;
                    switch (error)
                    {
                        case Error::BadRequest:
                            qCWarning(LOG) << "BadRequest" << errorString;
                            break;

                        case Error::Unauthorized:
                            qCWarning(LOG) << "Unauthorized" << errorString;
                            break;

                        case Error::Forbidden:
                            qCWarning(LOG) << "Forbidden" << errorString;
                            boost::asio::post(this->ioContext, [this, request] {
                                this->retrySubscription(
                                    request, boost::posix_time::seconds(2), 5);
                            });
                            break;

                        case Error::Conflict:
                            // This session ID is already subscribed to this request, some logic of ours is wrong
                            qCWarning(LOG) << "Conflict" << errorString;
                            break;

                        case Error::Ratelimited:
                            qCWarning(LOG) << "Ratelimited" << errorString;
                            break;

                        case Error::Forwarded:
                        default:
                            qCWarning(LOG)
                                << "Unhandled error, retrying subscription"
                                << errorString;
                            boost::asio::post(this->ioContext, [this, request] {
                                this->retrySubscription(
                                    request, boost::posix_time::seconds(2), 5);
                            });
                            break;
                    }
                });
            return;
        }

        if (openButNotReadyConnections == 0)
        {
            // No connection was available to handle this subscription request, create a new connection
            this->createConnection();
            this->retrySubscription(request, boost::posix_time::millisec(500),
                                    10);
        }
        else
        {
            if (openButNotReadyConnections > 1)
            {
                qCWarning(LOG) << "We have" << openButNotReadyConnections
                               << "open but no ready connections, hmmm";
            }

            // At least one connection is open, but it has not gotten the welcome message yet
            this->retrySubscription(request, boost::posix_time::millisec(250),
                                    10);
        }
    });
}

void Controller::createConnection()
{
    try
    {
        boost::asio::ssl::context sslContext{
            boost::asio::ssl::context::tlsv12_client};

        if constexpr (!LOCAL_EVENTSUB)
        {
            sslContext.set_verify_mode(
                boost::asio::ssl::verify_peer |
                boost::asio::ssl::verify_fail_if_no_peer_cert);
            sslContext.set_default_verify_paths();

            boost::certify::enable_native_https_server_verification(sslContext);
        }

        auto connection = std::make_shared<lib::Session>(
            this->ioContext, sslContext, std::make_unique<Connection>());

        this->registerConnection(connection);

        connection->run(this->eventSubHost, this->eventSubPort,
                        this->eventSubPath, this->userAgent);
    }
    catch (std::exception &e)
    {
        qCWarning(LOG) << "Error in EventSub run thread" << e.what();
    }
}

void Controller::registerConnection(std::weak_ptr<lib::Session> &&connection)
{
    this->threadGuard->guard();

    this->connections.emplace_back(std::move(connection));
}

void Controller::retrySubscription(const SubscriptionRequest &request,
                                   boost::posix_time::time_duration delay,
                                   int32_t maxAttempts)
{
    std::lock_guard lock(this->subscriptionsMutex);

    auto &connection = this->subscriptions[request];

    if (connection.retryAttempts <= 0)
    {
        connection.retryAttempts = maxAttempts;
    }
    else if (--connection.retryAttempts == 0)
    {
        qCWarning(LOG) << "Reached max amount of retries for" << request;
        return;
    }

    qCInfo(LOG) << "Retrying subscription" << request << " - attempt"
                << connection.retryAttempts;
    auto retryTimer =
        std::make_unique<boost::asio::deadline_timer>(this->ioContext);
    retryTimer->expires_from_now(delay);
    retryTimer->async_wait([this, request](const auto &ec) {
        if (!ec)
        {
            // The timer passed naturally
            this->subscribe(request, true);
        }
    });

    assert(connection.retryTimer == nullptr &&
           "Timer should not already be set");

    connection.retryTimer = std::move(retryTimer);
}

void Controller::markRequestSubscribed(const SubscriptionRequest &request,
                                       std::weak_ptr<lib::Session> connection,
                                       const QString &subscriptionID)
{
    std::lock_guard lock(this->subscriptionsMutex);

    auto &subscription = this->subscriptions[request];

    subscription.connection = std::move(connection);
    subscription.subscriptionID = subscriptionID;
}

}  // namespace chatterino::eventsub
