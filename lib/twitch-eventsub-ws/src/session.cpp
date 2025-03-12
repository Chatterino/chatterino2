#include "twitch-eventsub-ws/session.hpp"

#include "twitch-eventsub-ws/detail/errors.hpp"
#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/messages/metadata.hpp"
#include "twitch-eventsub-ws/payloads/channel-ban-v1.hpp"
#include "twitch-eventsub-ws/payloads/session-welcome.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

namespace chatterino::eventsub::lib {

// Subscription Type + Subscription Version
using EventSubSubscription = std::pair<std::string, std::string>;

using NotificationHandlers = std::unordered_map<
    EventSubSubscription,
    std::function<boost::system::error_code(const messages::Metadata &,
                                            const boost::json::value &,
                                            std::unique_ptr<Listener> &)>,
    boost::hash<EventSubSubscription>>;

namespace {

    template <class T>
    boost::system::result<T> parsePayload(const boost::json::value &jv)
    {
        auto result = boost::json::try_value_to<T>(jv);
        if (!result.has_value())
        {
            return result.error();
        }

        return std::move(result.value());
    }

    // Subscription types
    const NotificationHandlers NOTIFICATION_HANDLERS{
        {
            {"channel.ban", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::channel_ban::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelBan(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"stream.online", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::stream_online::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onStreamOnline(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"stream.offline", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::stream_offline::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onStreamOffline(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.chat.notification", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload = parsePayload<
                    payload::channel_chat_notification::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelChatNotification(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.update", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::channel_update::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelUpdate(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.chat.message", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::channel_chat_message::v1::Payload>(
                        jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelChatMessage(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.moderate", "2"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::channel_moderate::v2::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelModerate(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"automod.message.hold", "2"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::automod_message_hold::v2::Payload>(
                        jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onAutomodMessageHold(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"automod.message.update", "2"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload =
                    parsePayload<payload::automod_message_update::v2::Payload>(
                        jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onAutomodMessageUpdate(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.suspicious_user.message", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload = parsePayload<
                    payload::channel_suspicious_user_message::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelSuspiciousUserMessage(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.suspicious_user.update", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload = parsePayload<
                    payload::channel_suspicious_user_update::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelSuspiciousUserUpdate(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.chat.user_message_hold", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload = parsePayload<
                    payload::channel_chat_user_message_hold::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelChatUserMessageHold(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        {
            {"channel.chat.user_message_update", "1"},
            [](const auto &metadata, const auto &jv, auto &listener) {
                auto oPayload = parsePayload<
                    payload::channel_chat_user_message_update::v1::Payload>(jv);
                if (!oPayload)
                {
                    return oPayload.error();
                }
                listener->onChannelChatUserMessageUpdate(metadata, *oPayload);
                return boost::system::error_code{};
            },
        },
        // Add your new subscription types above this line
    };

}  // namespace

// Resolver and socket require an io_context
Session::Session(boost::asio::io_context &ioc, boost::asio::ssl::context &ctx,
                 std::unique_ptr<Listener> listener)
    : resolver(boost::asio::make_strand(ioc))
    , ws(boost::asio::make_strand(ioc), ctx)
    , listener(std::move(listener))
{
}

// Start the asynchronous operation
void Session::run(std::string _host, std::string _port, std::string _path,
                  std::string _userAgent)
{
    // Save these for later
    this->host = std::move(_host);
    this->port = std::move(_port);
    this->path = std::move(_path);
    this->userAgent = std::move(_userAgent);

    // Look up the domain name
    this->resolver.async_resolve(
        this->host, this->port,
        beast::bind_front_handler(&Session::onResolve, shared_from_this()));
}

void Session::close()
{
    boost::beast::websocket::close_reason closeReason("Shutting down");

    // TODO: Test this with a misbehaving eventsub server that doesn't respond to our close
    this->ws.async_close(
        closeReason,
        beast::bind_front_handler(&Session::onClose, shared_from_this()));
}

Listener *Session::getListener()
{
    return this->listener.get();
}

void Session::onResolve(
    beast::error_code ec,
    const boost::asio::ip::tcp::resolver::results_type &results)
{
    if (ec)
    {
        this->fail(ec, "resolve");
        return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(this->ws).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(this->ws).async_connect(
        results,
        beast::bind_front_handler(&Session::onConnect, shared_from_this()));
}

void Session::onConnect(
    beast::error_code ec,
    const boost::asio::ip::tcp::resolver::results_type::endpoint_type &ep)
{
    if (ec)
    {
        this->fail(ec, "connect");
        return;
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(this->ws).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(this->ws.next_layer().native_handle(),
                                  this->host.c_str()))
    {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                               boost::asio::error::get_ssl_category());
        this->fail(ec, "connect");
        return;
    }

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    this->ws.next_layer().async_handshake(
        boost::asio::ssl::stream_base::client,
        beast::bind_front_handler(&Session::onSSLHandshake,
                                  shared_from_this()));
}

void Session::onSSLHandshake(beast::error_code ec)
{
    if (ec)
    {
        this->fail(ec, "ssl_handshake");
        return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(this->ws).expires_never();

    // Set suggested timeout settings for the websocket
    this->ws.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    this->ws.set_option(websocket::stream_base::decorator(
        [userAgent{this->userAgent}](websocket::request_type &req) {
            req.set(http::field::user_agent, userAgent);
        }));

    // Perform the websocket handshake
    this->ws.async_handshake(
        this->host, this->path,
        beast::bind_front_handler(&Session::onHandshake, shared_from_this()));
}

void Session::onHandshake(beast::error_code ec)
{
    if (ec)
    {
        this->fail(ec, "handshake");
        return;
    }

    this->ws.async_read(buffer, beast::bind_front_handler(&Session::onRead,
                                                          shared_from_this()));
}

void Session::onRead(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (!this->listener)
    {
        return;
    }

    if (ec)
    {
        this->fail(ec, "read");
        return;
    }

    this->receivedMessage = true;
    auto messageError = this->handleMessage(this->buffer);
    if (messageError)
    {
        this->fail(messageError, "handleMessage");
    }

    this->buffer.clear();

    if (!this->listener)
    {
        this->close();
        return;
    }

    this->ws.async_read(buffer, beast::bind_front_handler(&Session::onRead,
                                                          shared_from_this()));
}

/**
    this->ws_.async_close(
        websocket::close_code::normal,
        beast::bind_front_handler(&Session::onClose, shared_from_this()));
        */
void Session::onClose(beast::error_code ec)
{
    if (ec)
    {
        this->fail(ec, "close");
        return;
    }

    // If we get here then the connection is closed gracefully
    if (this->listener)
    {
        this->listener->onClose(std::move(this->listener), {});
    }
}

void Session::fail(beast::error_code ec, std::string_view op)
{
    std::cerr << op << ": " << ec.message() << " (" << ec.location() << ")\n";
    if (!this->ws.is_open() && this->listener)
    {
        if (this->keepaliveTimer)
        {
            this->keepaliveTimer.reset();
        }

        this->listener->onClose(std::move(this->listener), {});
    }
}

boost::system::error_code Session::handleMessage(
    const beast::flat_buffer &buffer)
{
    boost::system::error_code parseError;
    auto jv =
        boost::json::parse(beast::buffers_to_string(buffer.data()), parseError);
    if (parseError)
    {
        // TODO: wrap error?
        return parseError;
    }

    const auto *jvObject = jv.if_object();
    if (jvObject == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::ExpectedObject);
    }

    const auto *metadataV = jvObject->if_contains("metadata");
    if (metadataV == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }
    auto metadataResult =
        boost::json::try_value_to<messages::Metadata>(*metadataV);
    if (metadataResult.has_error())
    {
        // TODO: wrap error?
        return metadataResult.error();
    }

    const auto &metadata = metadataResult.value();

    const auto *payloadV = jvObject->if_contains("payload");
    if (payloadV == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    if (metadata.messageType == "notification")
    {
        return this->onNotification(metadata, *payloadV);
    }
    if (metadata.messageType == "session_welcome")
    {
        return this->onSessionWelcome(metadata, *payloadV);
    }
    if (metadata.messageType == "session_keepalive")
    {
        return {};  // nothing to do
    }
    if (metadata.messageType == "session_reconnect")
    {
        return this->onSessionReconnect(*payloadV);
    }
    EVENTSUB_BAIL_HERE(error::Kind::NoMessageHandler);
}

boost::system::error_code Session::onSessionWelcome(
    const messages::Metadata &metadata, const boost::json::value &jv)
{
    auto oPayload = parsePayload<payload::session_welcome::Payload>(jv);
    if (!oPayload)
    {
        // TODO: error handling
        return oPayload.error();
    }
    const auto &payload = *oPayload;

    listener->onSessionWelcome(metadata, payload);

    // we're graceful with the keepalive timeout
    this->keepaliveTimeout =
        std::chrono::seconds{payload.keepaliveTimeoutSeconds.value_or(60)} * 2;
    assert(!this->keepaliveTimer);
    std::cerr << "Keepalive: " << this->keepaliveTimeout.count() << 's';
    this->checkKeepalive();

    return {};
}

boost::system::error_code Session::onSessionReconnect(
    const boost::json::value &jv)
{
    auto oPayload = parsePayload<payload::session_welcome::Payload>(jv);
    if (!oPayload)
    {
        return oPayload.error();
    }
    const auto &payload = *oPayload;
    auto *listenerPtr = listener.get();

    listenerPtr->onClose(std::move(listener), payload.reconnectURL);
    return {};
}

boost::system::error_code Session::onNotification(
    const messages::Metadata &metadata, const boost::json::value &jv)
{
    listener->onNotification(metadata, jv);

    if (!metadata.subscriptionType || !metadata.subscriptionVersion)
    {
        // TODO: error handling
        return boost::system::error_code{};
    }

    auto it = NOTIFICATION_HANDLERS.find(
        {*metadata.subscriptionType, *metadata.subscriptionVersion});
    if (it == NOTIFICATION_HANDLERS.end())
    {
        EVENTSUB_BAIL_HERE(error::Kind::NoMessageHandler);
    }

    return it->second(metadata, jv, listener);
}

void Session::checkKeepalive()
{
    if (!this->receivedMessage)
    {
        std::cerr << "Keepalive timeout, closing\n";
        if (this->listener)
        {
            this->listener->onClose(std::move(this->listener), {});
        }
        this->close();
        return;
    }
    this->receivedMessage = false;

    if (this->keepaliveTimeout.count() == 0)
    {
        return;
    }

    this->keepaliveTimer =
        std::make_unique<boost::asio::system_timer>(this->ws.get_executor());
    this->keepaliveTimer->expires_after(this->keepaliveTimeout);
    this->keepaliveTimer->async_wait([this](boost::system::error_code ec) {
        if (ec)
        {
            std::cerr << "Keepalive timer cancelled: " << ec.message() << '\n';
            return;
        }
        this->checkKeepalive();
    });
}

}  // namespace chatterino::eventsub::lib
