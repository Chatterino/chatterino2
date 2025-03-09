#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/json.hpp>

namespace chatterino::eventsub::lib::messages {

struct Metadata;

}  // namespace chatterino::eventsub::lib::messages

namespace chatterino::eventsub::lib {

class Listener;

class Session : public std::enable_shared_from_this<Session>
{
public:
    // Resolver and socket require an io_context
    explicit Session(boost::asio::io_context &ioc,
                     boost::asio::ssl::context &ctx,
                     std::unique_ptr<Listener> listener);

    // Start the asynchronous operation
    void run(std::string _host, std::string _port, std::string _path,
             std::string _userAgent);

    void close();

    Listener *getListener();

    // public for testing
    boost::system::error_code handleMessage(
        const boost::beast::flat_buffer &buffer);

private:
    void onResolve(boost::beast::error_code ec,
                   const boost::asio::ip::tcp::resolver::results_type &results);

    void onConnect(
        boost::beast::error_code ec,
        const boost::asio::ip::tcp::resolver::results_type::endpoint_type &ep);

    void onSSLHandshake(boost::beast::error_code ec);

    void onHandshake(boost::beast::error_code ec);

    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);

    void onClose(boost::beast::error_code ec);

    void fail(boost::beast::error_code ec, std::string_view op);

    boost::system::error_code onSessionWelcome(
        const messages::Metadata &metadata, const boost::json::value &jv);
    boost::system::error_code onSessionReconnect(const boost::json::value &jv);
    boost::system::error_code onNotification(const messages::Metadata &metadata,
                                             const boost::json::value &jv);

    void checkKeepalive();

    boost::asio::ip::tcp::resolver resolver;
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>>
        ws;
    boost::beast::flat_buffer buffer;
    std::string host;
    std::string port;
    std::string path;
    std::string userAgent;
    std::unique_ptr<Listener> listener;

    std::chrono::seconds keepaliveTimeout{0};
    bool receivedMessage = false;
    std::unique_ptr<boost::asio::system_timer> keepaliveTimer;
};

}  // namespace chatterino::eventsub::lib
