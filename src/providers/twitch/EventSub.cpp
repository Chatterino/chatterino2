#include "providers/twitch/EventSub.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "eventsub/listener.hpp"
#include "eventsub/payloads/channel-ban-v1.hpp"
#include "eventsub/payloads/session-welcome.hpp"
#include "eventsub/session.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/PostToThread.hpp"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/json.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
// using namespace boost::asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

using WebSocketStream = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

namespace chatterino {

using namespace eventsub;

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

awaitable<void> session(WebSocketStream &ws, std::unique_ptr<Listener> listener)
{
    // start reader
    std::cout << "start reader\n";
    co_await (sessionReader(ws, std::move(listener)));
    // co_spawn(ws.get_executor(), sessionReader(ws), detached);
    std::cout << "reader stopped\n";

    co_return;
}

class MyListener final : public Listener
{
public:
    void onSessionWelcome(messages::Metadata metadata,
                          payload::session_welcome::Payload payload) override
    {
        (void)metadata;
        std::cout << "ON session welcome " << payload.id << " XD\n";

        auto sessionID = QString::fromStdString(payload.id);

        const auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            return;
        }

        auto sourceUserID = currentUser->getUserId();

        getApp()->twitch->forEachChannelAndSpecialChannels(
            [sessionID, sourceUserID](const ChannelPtr &channel) {
                if (channel->getType() == Channel::Type::Twitch)
                {
                    auto *twitchChannel =
                        dynamic_cast<TwitchChannel *>(channel.get());

                    auto roomID = twitchChannel->roomId();

                    if (channel->isBroadcaster())
                    {
                        QJsonObject condition;
                        condition.insert("broadcaster_user_id", roomID);

                        getHelix()->createEventSubSubscription(
                            "channel.ban", "1", sessionID, condition,
                            [roomID](const auto &response) {
                                qDebug() << "Successfully subscribed to "
                                            "channel.ban in"
                                         << roomID << ":" << response;
                            },
                            [roomID](auto error, const auto &message) {
                                (void)error;
                                qDebug()
                                    << "Failed subscription to channel.ban in"
                                    << roomID << ":" << message;
                            });
                    }

                    {
                        QJsonObject condition;
                        condition.insert("broadcaster_user_id", roomID);
                        condition.insert("user_id", sourceUserID);

                        getHelix()->createEventSubSubscription(
                            "channel.chat.notification", "1", sessionID,
                            condition,
                            [roomID](const auto &response) {
                                qDebug() << "Successfully subscribed to "
                                            "channel.chat.notification in "
                                         << roomID << ":" << response;
                            },
                            [roomID](auto error, const auto &message) {
                                (void)error;
                                qDebug() << "Failed subscription to "
                                            "channel.chat.notification in"
                                         << roomID << ":" << message;
                            });
                    }
                }
            });
    }

    void onNotification(messages::Metadata metadata,
                        const boost::json::value &jv) override
    {
        (void)metadata;
        std::cout << "on notification: " << jv << '\n';
    }

    void onChannelBan(messages::Metadata metadata,
                      payload::channel_ban::v1::Payload payload) override
    {
        (void)metadata;
        std::cout << "Channel ban occured in "
                  << payload.event.broadcasterUserLogin << "'s channel:"
                  << " isPermanent=" << payload.event.isPermanent
                  << " reason=" << payload.event.reason
                  << " userLogin=" << payload.event.userLogin
                  << " moderatorLogin=" << payload.event.moderatorUserLogin
                  << " bannedAt=" << payload.event.bannedAt << '\n';

        auto roomID = QString::fromStdString(payload.event.broadcasterUserID);

        BanAction action{
            //
        };

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
            qDebug() << "TIMEOUT DURATION IN SECONDS: "
                     << timeoutDurationInSeconds;
        }

        auto chan = getApp()->twitch->getChannelOrEmptyByID(roomID);

        runInGuiThread([action{std::move(action)}, chan{std::move(chan)}] {
            MessageBuilder msg(action);
            msg->flags.set(MessageFlag::PubSub);
            chan->addOrReplaceTimeout(msg.release());
        });
    }

    void onStreamOnline(messages::Metadata metadata,
                        payload::stream_online::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "ON STREAM ONLINE XD\n";
    }

    void onStreamOffline(messages::Metadata metadata,
                         payload::stream_offline::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "ON STREAM OFFLINE XD\n";
    }

    void onChannelChatNotification(
        messages::Metadata metadata,
        payload::channel_chat_notification::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Received channel.chat.notification v1\n";
    }

    void onChannelUpdate(messages::Metadata metadata,
                         payload::channel_update::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Channel update event!\n";
    }

    // Add your new subscription types above this line
};

awaitable<void> connectToClient(boost::asio::io_context &ioContext,
                                const std::string host, const std::string port,
                                const std::string path,
                                boost::asio::ssl::context &sslContext)
{
    auto tcpResolver = tcp::resolver(ioContext);

    for (;;)
    {
        // TODO: wait on (AND INCREMENT) backoff timer

        boost::system::error_code resolveError;
        auto target = co_await tcpResolver.async_resolve(
            host, port,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        resolveError));

        std::cout << "Connecting to " << host << ":" << port << "\n";
        if (resolveError)
        {
            fail(resolveError, "resolve");
            continue;
        }

        WebSocketStream ws(ioContext, sslContext);

        // Make the connection on the IP address we get from a lookup
        // TODO: Check connectError
        boost::system::error_code connectError;
        auto endpoint = co_await beast::get_lowest_layer(ws).async_connect(
            target, boost::asio::redirect_error(boost::asio::use_awaitable,
                                                connectError));

        std::string hostHeader{host};

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(),
                                      host.data()))
        {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category());
            fail(ec, "connect");
            continue;
        }

        // Update the host string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        hostHeader += ':' + std::to_string(endpoint.port());

        // Set a timeout on the operation
        beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(
            websocket::stream_base::decorator([](websocket::request_type &req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-coro");
            }));

        // Perform the SSL handshake
        boost::system::error_code sslHandshakeError;
        co_await ws.next_layer().async_handshake(
            ssl::stream_base::client,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        sslHandshakeError));
        if (sslHandshakeError)
        {
            fail(sslHandshakeError, "ssl_handshake");
            continue;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws).expires_never();

        // Set suggested timeout settings for the websocket
        ws.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));

        // Perform the websocket handshake
        boost::system::error_code wsHandshakeError;
        co_await ws.async_handshake(
            hostHeader, path,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        wsHandshakeError));
        if (wsHandshakeError)
        {
            fail(wsHandshakeError, "handshake");
            continue;
        }

        std::unique_ptr<Listener> listener = std::make_unique<MyListener>();
        co_await session(ws, std::move(listener));

        // Close the WebSocket connection
        boost::system::error_code closeError;
        co_await ws.async_close(websocket::close_code::normal,
                                boost::asio::redirect_error(
                                    boost::asio::use_awaitable, closeError));
        if (closeError)
        {
            fail(closeError, "close");
        }
        else
        {
            // If we get here then the connection is closed gracefully
            std::cout << "Closed connection gracefully\n";
        }

        // TODO: reset backoff timer
    }
}

void EventSub::start()
{
    // for use with twitch CLI: twitch event websocket start-server --ssl --port 3012
    // const auto *const host = "localhost";
    // const auto *const port = "3012";
    // const auto *const path = "/ws";

    // for use with real Twitch eventsub
    std::string host{"eventsub.wss.twitch.tv"};
    std::string port("443");
    std::string path("/ws");

    try
    {
        this->mainThread = std::make_unique<std::thread>([=] {
            boost::asio::io_context ctx(1);

            boost::asio::ssl::context sslContext{
                boost::asio::ssl::context::tlsv12_client};

            // TODO: Load certificates into SSL context

            co_spawn(ctx, connectToClient(ctx, host, port, path, sslContext),
                     detached);
            ctx.run();
        });
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

}  // namespace chatterino
