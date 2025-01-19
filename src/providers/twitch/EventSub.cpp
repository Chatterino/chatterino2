#include "providers/twitch/EventSub.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
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

class MyListener final : public eventsub::Listener
{
public:
    void onSessionWelcome(
        eventsub::messages::Metadata metadata,
        eventsub::payload::session_welcome::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On session welcome:" << payload.id.c_str();

        auto sessionID = QString::fromStdString(payload.id);

        const auto currentUser = getApp()->getAccounts()->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            return;
        }

        auto sourceUserID = currentUser->getUserId();
    }

    void onNotification(eventsub::messages::Metadata metadata,
                        const boost::json::value &jv) override
    {
        (void)metadata;
        auto jsonString = boost::json::serialize(jv);
        qCDebug(LOG) << "on notification: " << jsonString.c_str();
    }

    void onChannelBan(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_ban::v1::Payload payload) override
    {
        (void)metadata;

        auto roomID = QString::fromStdString(payload.event.broadcasterUserID);

        BanAction action{};

        action.timestamp = std::chrono::steady_clock::now();
        action.roomID = roomID;
        action.source = ActionUser{
            .id = QString::fromStdString(payload.event.moderatorUserID),
            .login = QString::fromStdString(payload.event.moderatorUserLogin),
            .displayName =
                QString::fromStdString(payload.event.moderatorUserName),
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
                std::chrono::duration_cast<std::chrono::seconds>(
                    timeoutDuration)
                    .count();
            action.duration = timeoutDurationInSeconds;
        }

        auto chan = getApp()->getTwitch()->getChannelOrEmptyByID(roomID);

        runInGuiThread([action{std::move(action)}, chan{std::move(chan)}] {
            MessageBuilder msg(action);
            msg->flags.set(MessageFlag::PubSub);
            chan->addOrReplaceTimeout(msg.release(),
                                      QDateTime::currentDateTime());
        });
    }

    void onStreamOnline(
        eventsub::messages::Metadata metadata,
        eventsub::payload::stream_online::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On stream online event for channel"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onStreamOffline(
        eventsub::messages::Metadata metadata,
        eventsub::payload::stream_offline::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On stream offline event for channel"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onChannelChatNotification(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_chat_notification::v1::Payload payload)
        override
    {
        (void)metadata;
        qCDebug(LOG) << "On channel chat notification for"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onChannelUpdate(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_update::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On channel update for"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onChannelChatMessage(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_chat_message::v1::Payload payload) override
    {
        (void)metadata;

        qCDebug(LOG) << "Channel chat message event for"
                     << payload.event.broadcasterUserLogin.c_str();
    }
};

EventSub::EventSub()
    : ioContext(1)
    , work(boost::asio::make_work_guard(this->ioContext))
{
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

void EventSub::createConnection()
{
    const auto userAgent = QStringLiteral("chatterino/%1 (%2)")
                               .arg(Version::instance().version(),
                                    Version::instance().commitHash())
                               .toUtf8()
                               .toStdString();

    auto eventSubHost = getEventSubHost();

    try
    {
        auto [host, port, path] = eventSubHost;

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

        std::make_shared<eventsub::Session>(this->ioContext, sslContext,
                                            std::make_unique<MyListener>())
            ->run(host, port, path, userAgent);
    }
    catch (std::exception &e)
    {
        qCWarning(LOG) << "Error in EventSub run thread" << e.what();
    }
}

}  // namespace chatterino
