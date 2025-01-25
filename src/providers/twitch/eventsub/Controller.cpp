#include "providers/twitch/eventsub/Controller.hpp"

#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/eventsub/Connection.hpp"
#include "util/RenameThread.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/certify/https_verification.hpp>
#include <twitch-eventsub-ws/session.hpp>

#include <memory>

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
}

Controller::~Controller()
{
    this->work.reset();

    // TODO: Close down existing sessions

    if (this->thread->joinable())
    {
        this->thread->join();
    }
    else
    {
        qCWarning(LOG) << "Thread not joinable";
    }
}

void Controller::subscribe(const SubscriptionRequest &request, bool isQueued)
{
    qCInfo(LOG) << "Subscribe request for" << request.subscriptionType;
    boost::asio::post(this->ioContext, [this, request, isQueued] {
        // 1. Flush dead connections (maybe this should not be done here)
        // TODO: implement

        if (isQueued)
        {
            qCInfo(LOG) << "Removing subscription from queued list";
            this->queuedSubscriptions.erase(request);
        }

        if (this->queuedSubscriptions.contains(request))
        {
            qCWarning(LOG) << "We already have a queued subscription for this, "
                              "let's chill :)";
            return;
        }

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

            // TODO: Check if this listener has room for another subscription
            // TODO: Check if this listener has a session ID yet

            // TODO: Don't hardcode the subscription version
            QJsonObject condition;
            for (const auto &[conditionKey, conditionValue] :
                 request.conditions)
            {
                condition.insert(conditionKey, conditionValue);
            }
            getHelix()->createEventSubSubscription(
                request.subscriptionType, request.subscriptionVersion,
                listener->getSessionID(), condition,
                [](const auto &res) {
                    qCInfo(LOG) << "Successfully subscribed!" << res;
                },
                [](const auto &error, const auto &errorString) {
                    qCWarning(LOG) << "Failed to subscribe" << errorString;
                    // TODO: retry?
                });
            return;
        }

        // No connection was available to handle this subscription request, create a new connection
        // TODO: Do we need to limit the amount of connections we create?
        this->createConnection();

        auto resubTimer =
            std::make_unique<boost::asio::deadline_timer>(this->ioContext);
        resubTimer->expires_from_now(boost::posix_time::seconds(2));
        resubTimer->async_wait([this, request](const auto &ec) {
            // TODO: Check what the EC is to know whether or not to actually fire the timer
            qCInfo(LOG) << "TIMER FIRED!";
            this->subscribe(request, true);
        });

        this->queuedSubscriptions.emplace(request, std::move(resubTimer));
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
    this->connections.emplace_back(std::move(connection));
}

}  // namespace chatterino::eventsub
