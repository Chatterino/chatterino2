#include "providers/twitch/EventSub.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/PostToThread.hpp"
#include "util/RenameThread.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/certify/https_verification.hpp>
#include <boost/json.hpp>
#include <twitch-eventsub-ws/listener.hpp>
#include <twitch-eventsub-ws/session.hpp>

#include <chrono>
#include <memory>

using namespace std::literals::chrono_literals;

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

namespace chatterino {

void EventSubClient::onSessionWelcome(
    eventsub::messages::Metadata metadata,
    eventsub::payload::session_welcome::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On session welcome:" << payload.id.c_str();

    this->sessionID = QString::fromStdString(payload.id);
}

void EventSubClient::onNotification(eventsub::messages::Metadata metadata,
                                    const boost::json::value &jv)
{
    (void)metadata;
    auto jsonString = boost::json::serialize(jv);
    qCDebug(LOG) << "on notification: " << jsonString.c_str();
}

void EventSubClient::onChannelBan(
    eventsub::messages::Metadata metadata,
    eventsub::payload::channel_ban::v1::Payload payload)
{
    (void)metadata;

    auto roomID = QString::fromStdString(payload.event.broadcasterUserID);

    BanAction action{};

    action.timestamp = std::chrono::steady_clock::now();
    action.roomID = roomID;
    action.source = ActionUser{
        .id = QString::fromStdString(payload.event.moderatorUserID),
        .login = QString::fromStdString(payload.event.moderatorUserLogin),
        .displayName = QString::fromStdString(payload.event.moderatorUserName),
    };
    action.target = ActionUser{
        .id = QString::fromStdString(payload.event.userID),
        .login = QString::fromStdString(payload.event.userLogin),
        .displayName = QString::fromStdString(payload.event.userName),
    };
    action.reason = QString::fromStdString(payload.event.reason);
    if (payload.event.isPermanent)
    {
        action.duration = 0;
    }
    else
    {
        auto timeoutDuration = payload.event.timeoutDuration();
        auto timeoutDurationInSeconds =
            std::chrono::duration_cast<std::chrono::seconds>(timeoutDuration)
                .count();
        action.duration = timeoutDurationInSeconds;
    }

    auto chan = getApp()->getTwitch()->getChannelOrEmptyByID(roomID);

    runInGuiThread([action{std::move(action)}, chan{std::move(chan)}] {
        auto time = QDateTime::currentDateTime();
        MessageBuilder msg(action, time);
        msg->flags.set(MessageFlag::PubSub);
        chan->addOrReplaceTimeout(msg.release(), QDateTime::currentDateTime());
    });
}

void EventSubClient::onStreamOnline(
    eventsub::messages::Metadata metadata,
    eventsub::payload::stream_online::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream online event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void EventSubClient::onStreamOffline(
    eventsub::messages::Metadata metadata,
    eventsub::payload::stream_offline::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On stream offline event for channel"
                 << payload.event.broadcasterUserLogin.c_str();
}

void EventSubClient::onChannelChatNotification(
    eventsub::messages::Metadata metadata,
    eventsub::payload::channel_chat_notification::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel chat notification for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void EventSubClient::onChannelUpdate(
    eventsub::messages::Metadata metadata,
    eventsub::payload::channel_update::v1::Payload payload)
{
    (void)metadata;
    qCDebug(LOG) << "On channel update for"
                 << payload.event.broadcasterUserLogin.c_str();
}

void EventSubClient::onChannelChatMessage(
    eventsub::messages::Metadata metadata,
    eventsub::payload::channel_chat_message::v1::Payload payload)
{
    (void)metadata;

    qCDebug(LOG) << "Channel chat message event for"
                 << payload.event.broadcasterUserLogin.c_str();
}

EventSub::EventSub()
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

EventSub::~EventSub()
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

void EventSub::subscribe(const SubscriptionRequest &request, bool isQueued)
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
                dynamic_cast<EventSubClient *>(connection->getListener());

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

void EventSub::createConnection()
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

        auto connection = std::make_shared<eventsub::Session>(
            this->ioContext, sslContext, std::make_unique<EventSubClient>());

        this->registerConnection(connection);

        connection->run(this->eventSubHost, this->eventSubPort,
                        this->eventSubPath, this->userAgent);
    }
    catch (std::exception &e)
    {
        qCWarning(LOG) << "Error in EventSub run thread" << e.what();
    }
}

void EventSub::registerConnection(std::weak_ptr<eventsub::Session> &&connection)
{
    this->connections.emplace_back(std::move(connection));
}

}  // namespace chatterino
