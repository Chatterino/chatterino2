#include "providers/twitch/eventsub/Controller.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
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

using namespace chatterino;

std::tuple<std::string, std::string, std::string> getEventSubHost()
{
#ifndef NDEBUG
    if (getApp()->getArgs().useLocalEventsub)
    {
        return {"localhost", "3012", "/ws"};
    }
#endif

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
    assert(this->quitting && "Application should call setQuitting() before "
                             "destroying the controller");

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
    if (this->quitting)
    {
        // We're quitting - we don't care to unsub
        return;
    }

    std::lock_guard lock(this->subscriptionsMutex);

    assert(this->subscriptions.contains(request));

    auto &subscription = this->subscriptions[request];
    subscription.refCount--;
    assert(subscription.refCount >= 0);
    if (subscription.refCount == 0)
    {
        qCDebug(LOG) << "Removed last ref for" << request;
    }
    else
    {
        qCDebug(LOG) << "Removed ref for" << request << "("
                     << subscription.refCount << "remaining)";
    }

    if (subscription.refCount <= 0)
    {
        // No longer interested in this topic, ensure we don't have a retry in flight
        subscription.retryTimer.reset();
        if (subscription.subscriptionID.isEmpty())
        {
            qCDebug(LOG)
                << "Refcount fell to zero for" << request
                << "but we had no subscription ID attached - a "
                   "successful subscription was never made. From state "
                << static_cast<uint8_t>(subscription.state);
            return;
        }

        qCDebug(LOG) << "Unsubscribing from" << request;
        subscription.state = Subscription::State::Unsubscribing;

        getHelix()->deleteEventSubSubscription(
            subscription.subscriptionID,
            [this, request] {
                qCDebug(LOG) << "Successfully unsubscribed from" << request;
                this->markRequestUnsubscribed(request);
            },
            [this, request](const auto &errorMessage) {
                qCWarning(LOG)
                    << "An error occurred while attempting to unsubscribe from"
                    << request << errorMessage;
                this->markRequestUnsubscribed(request);
            });

        subscription.subscriptionID.clear();
    }
}

void Controller::setQuitting()
{
    this->quitting = true;
}

SubscriptionHandle Controller::subscribe(const SubscriptionRequest &request)
{
    assert(!this->quitting &&
           "Subscribe cannot be called while we are quitting");

    bool needToSubscribe = false;

    {
        // TODO: Investigate if this scope can be done in boost::asio::post instead
        // Basically, if the SubscriptionHandle can be built & returned entirely without waiting for the subscriptionsMutex lock
        std::lock_guard lock(this->subscriptionsMutex);

        auto &subscription = this->subscriptions[request];

        assert(subscription.refCount >= 0);

        switch (subscription.state)
        {
            case Subscription::State::Unsubscribed:
                needToSubscribe = true;
                assert(subscription.refCount == 0 &&
                       "An unsubscribed subscription should have 0 references");
                break;

            case Subscription::State::Failed:
                qCDebug(LOG)
                    << "New subscription attempt to previously-failed request"
                    << request;
                needToSubscribe = true;
                break;

            case Subscription::State::Subscribing:
            case Subscription::State::Retrying:
            case Subscription::State::Subscribed:
            case Subscription::State::Unsubscribing:
                break;
        }

        if (needToSubscribe)
        {
            qCDebug(LOG) << "Set state to subscribing" << request;
            subscription.state = Subscription::State::Subscribing;

            // Ensure retries can work as expected since this is a fresh subscription
            subscription.backoff.reset();

            assert(subscription.retryTimer == nullptr &&
                   "A new subscription should not have a retry timer created");
        }

        subscription.refCount++;
        qCDebug(LOG) << "Added ref for" << request << subscription.refCount
                     << needToSubscribe
                     << "state:" << static_cast<uint8_t>(subscription.state);
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

void Controller::reconnectConnection(
    std::unique_ptr<lib::Listener> connection,
    const std::optional<std::string> &reconnectURL,
    const std::unordered_set<SubscriptionRequest> &subs)
{
    this->clearConnections();
    if (subs.empty())
    {
        return;
    }

    if (reconnectURL)
    {
        qCDebug(chatterinoTwitchEventSub) << "Using reconnect URL to reconnect";
        // this is epic
        QUrl url(QString::fromStdString(*reconnectURL));
        this->createConnection(url.host(QUrl::FullyEncoded).toStdString(),
                               std::to_string(url.port(443)),
                               url.path(QUrl::FullyEncoded).toStdString(),
                               std::move(connection));
        return;
    }

    // no reconnect URL - something happened
    // but first, clear the subscriptions
    qCDebug(chatterinoTwitchEventSub)
        << "Resubscribing to topics after connection failure";
    {
        std::lock_guard g(this->subscriptionsMutex);
        for (const auto &sub : subs)
        {
            auto it = this->subscriptions.find(sub);
            if (it != this->subscriptions.end())
            {
                qCDebug(chatterinoTwitchEventSub) << "Resetting" << it->first;
                it->second.connection = {};
                it->second.backoff.reset();
                it->second.state = Subscription::State::Subscribing;
            }
        }
    }

    for (const auto &sub : subs)
    {
        this->subscribe(sub, false);
    }
}

void Controller::subscribe(const SubscriptionRequest &request, bool isRetry)
{
    // 1. Flush dead connections (maybe this should not be done here)
    // TODO: implement

    {
        std::lock_guard lock(this->subscriptionsMutex);
        auto &subscription = this->subscriptions[request];
        if (isRetry)
        {
            qCDebug(LOG) << "Retry subscribe request for" << request;

            assert(subscription.retryTimer != nullptr);

            subscription.retryTimer.reset();
        }
        else
        {
            qCDebug(LOG) << "New subscribe request for" << request;
        }

        assert(subscription.retryTimer == nullptr);
    }

    uint32_t openButNotReadyConnections = 0;

    // 2. Check if any currently open connection can handle this subscription
    auto viableConnection =
        this->getViableConnection(openButNotReadyConnections);

    if (viableConnection.has_value())
    {
        const auto &connection = *viableConnection;
        auto *listener = dynamic_cast<Connection *>(connection->getListener());

        assert(listener != nullptr && "Something goofy has gone wrong, Session "
                                      "listener must be our Connection type");

        qCDebug(LOG) << "Make helix request for" << request;
        getHelix()->createEventSubSubscription(
            request, listener->getSessionID(),
            [this, request, connection,
             weakConnection{std::weak_ptr<lib::Session>(connection)}](
                const auto &res) {
                qCDebug(LOG) << "Subscription success" << request;
                this->markRequestSubscribed(request, weakConnection,
                                            res.subscriptionID);
            },
            [this, request](const auto &error, const auto &errorString) {
                using Error = HelixCreateEventSubSubscriptionError;
                switch (error)
                {
                    case Error::BadRequest:
                        qCDebug(LOG) << "Bad request" << errorString << request;
                        break;

                    case Error::Unauthorized:
                        qCDebug(LOG)
                            << "Unauthorized" << errorString << request;
                        break;

                    case Error::Forbidden:
                        qCDebug(LOG) << "Forbidden" << errorString << request;
                        return;

                    case Error::Conflict:
                        // This session ID is already subscribed to this request, some logic of ours is wrong
                        qCWarning(LOG) << "Conflict" << errorString << request;
                        break;

                    case Error::Ratelimited:
                        qCDebug(LOG) << "Ratelimited" << errorString << request;
                        break;

                    case Error::Forwarded:
                    default:
                        qCWarning(LOG) << "Unhandled error, retrying "
                                          "subscription"
                                       << errorString << request;
                        boost::asio::post(this->ioContext, [this, request] {
                            this->retrySubscription(request);
                        });
                        return;
                }

                this->markRequestFailed(request);
            });

        return;
    }

    if (openButNotReadyConnections == 0)
    {
        // No connection was available to handle this subscription request, create a new connection
        this->createConnection();
    }
    else if (openButNotReadyConnections > 1)
    {
        // There should only ever be 0 or 1
        qCWarning(LOG) << "We have" << openButNotReadyConnections
                       << "open but no ready connections";
    }

    this->retrySubscription(request);
}

std::optional<std::shared_ptr<lib::Session>> Controller::getViableConnection(
    uint32_t &openButNotReadyConnections)
{
    for (const auto &weakConnection : this->connections)
    {
        auto connection = weakConnection.lock();
        if (!connection)
        {
            // TODO: remove it here?
            continue;
        }

        auto *listener = dynamic_cast<Connection *>(connection->getListener());

        if (!listener)
        {
            continue;  // dead connection
        }

        if (listener->getSessionID().isEmpty())
        {
            // This connection is open but it's not ready (i.e. no welcome has been posted yet)
            ++openButNotReadyConnections;
            continue;
        }

        // TODO: Check if this listener has room for another subscription

        return connection;
    }

    return {};
}

void Controller::createConnection()
{
    this->createConnection(this->eventSubHost, this->eventSubPort,
                           this->eventSubPath, std::make_unique<Connection>());
}

void Controller::createConnection(std::string host, std::string port,
                                  std::string path,
                                  std::unique_ptr<lib::Listener> listener)
{
    try
    {
        boost::asio::ssl::context sslContext{
            boost::asio::ssl::context::tlsv12_client};

#ifndef NDEBUG
        if (!getApp()->getArgs().useLocalEventsub)
#endif
        {
            sslContext.set_verify_mode(
                boost::asio::ssl::verify_peer |
                boost::asio::ssl::verify_fail_if_no_peer_cert);
            sslContext.set_default_verify_paths();

            boost::certify::enable_native_https_server_verification(sslContext);
        }

        auto connection = std::make_shared<lib::Session>(
            this->ioContext, sslContext, std::move(listener));

        this->registerConnection(connection);

        connection->run(std::move(host), std::move(port), std::move(path),
                        this->userAgent);
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

void Controller::retrySubscription(const SubscriptionRequest &request)
{
    std::lock_guard lock(this->subscriptionsMutex);

    auto &subscription = this->subscriptions[request];

    if (subscription.refCount == 0)
    {
        qCDebug(LOG) << "No one is interested in this subscription anymore, "
                        "stop trying"
                     << request << "from state"
                     << static_cast<uint8_t>(subscription.state);
        qCDebug(LOG) << "Set state to unsubscribed" << request;
        subscription.state = Subscription::State::Unsubscribed;
        return;
    }

    if (subscription.state != Subscription::State::Retrying)
    {
        qCDebug(LOG) << "Set state to retrying" << request;
        subscription.state = Subscription::State::Retrying;
    }

    // we don't need a strong RNG here
    // NOLINTNEXTLINE(cert-*)
    std::chrono::milliseconds jitter{std::rand() % 256};

    auto retryTimer =
        std::make_unique<boost::asio::system_timer>(this->ioContext);
    retryTimer->expires_after(subscription.backoff.next() + jitter);
    retryTimer->async_wait([this, request](const auto &ec) {
        if (!ec)
        {
            qCDebug(LOG) << "Firing retry" << request;
            // The timer passed naturally
            this->subscribe(request, true);
        }
        else
        {
            qCDebug(LOG) << "Retry timer for" << request << "was cancelled";
            // If we mark the request as unsubscribed here, and we had to actually unsubscribe,
            // if an actual unsubscribe happens then it'll go from Unsubscribed -> ACTUALLY unsubscribed
            //
            // We might still need to update the state here, but we might want some new state for that
            // e.g. RetryCancelled or something
            // this->markRequestUnsubscribed(request);
        }
    });

    assert(subscription.retryTimer == nullptr &&
           "Timer should not already be set");

    subscription.retryTimer = std::move(retryTimer);
}

void Controller::markRequestSubscribed(const SubscriptionRequest &request,
                                       std::weak_ptr<lib::Session> connection,
                                       const QString &subscriptionID)
{
    if (this->quitting)
    {
        return;
    }

    std::lock_guard lock(this->subscriptionsMutex);

    auto strong = connection.lock();
    if (!strong)
    {
        // we disconnected
        return;
    }
    auto *listener = dynamic_cast<Connection *>(strong->getListener());
    if (!listener)
    {
        // we disconnected
        return;
    }
    listener->markRequestSubscribed(request);

    auto &subscription = this->subscriptions[request];

    assert((subscription.state == Subscription::State::Subscribing ||
            subscription.state == Subscription::State::Retrying) &&
           "A subscription can only be marked subscribed from the Subscribing "
           "or Retrying state");

    subscription.connection = std::move(connection);
    subscription.subscriptionID = subscriptionID;
    qCDebug(LOG) << "Set state to subscribed" << request;
    subscription.state = Subscription::State::Subscribed;
    subscription.backoff.reset();
}

void Controller::markRequestFailed(const SubscriptionRequest &request)
{
    if (this->quitting)
    {
        return;
    }

    qCWarning(LOG) << "Request" << request << "marked as failed";

    std::lock_guard lock(this->subscriptionsMutex);

    auto &subscription = this->subscriptions[request];

    qCDebug(LOG) << "Set state to failed" << request;
    subscription.state = Subscription::State::Failed;
}

void Controller::markRequestUnsubscribed(const SubscriptionRequest &request)
{
    if (this->quitting)
    {
        return;
    }

    std::lock_guard lock(this->subscriptionsMutex);

    auto &subscription = this->subscriptions[request];

    qCDebug(LOG) << "Request" << request << "marked as unsubscribed from state"
                 << static_cast<uint8_t>(subscription.state);

    assert(subscription.state == Subscription::State::Unsubscribing ||
           subscription.state == Subscription::State::Retrying);
    assert(subscription.retryTimer == nullptr);

    qCDebug(LOG) << "Set state to unsubscribed" << request;
    subscription.state = Subscription::State::Unsubscribed;
    subscription.backoff.reset();
    auto conn = subscription.connection.lock();
    if (conn)
    {
        auto *listener = dynamic_cast<Connection *>(conn->getListener());
        if (listener)
        {
            listener->markRequestUnsubscribed(request);
        }
    }
    subscription.connection = {};

    if (subscription.refCount == 0)
    {
        // we could remove the subscription here
        return;
    }

    // someone subscribed in the meantime
    subscription.state = Subscription::State::Subscribing;
    boost::asio::post(this->ioContext, [this, request] {
        this->subscribe(request, false);
    });
}

void Controller::clearConnections()
{
    std::erase_if(this->connections, [](const auto &it) {
        auto conn = it.lock();
        return !conn || !conn->getListener();
    });
}

void DummyController::reconnectConnection(
    std::unique_ptr<lib::Listener> /* connection */,
    const std::optional<std::string> & /* reconnectURL */,
    const std::unordered_set<SubscriptionRequest> & /* subs */)
{
}

}  // namespace chatterino::eventsub
