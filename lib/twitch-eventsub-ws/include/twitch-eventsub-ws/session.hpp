#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/json.hpp>

namespace chatterino::eventsub::lib {

class Listener;

/**
 * handleMessage takes the incoming message in the buffer, parses it
 * as JSON then forwards it to the listener, if applicable.
 *
 * This is called from the Session, and is only provided if you are interested
 * in building your own boost asio framework thing
 **/
boost::system::error_code handleMessage(
    std::unique_ptr<Listener> &listener,
    const boost::beast::flat_buffer &buffer);

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

private:
    void onResolve(boost::beast::error_code ec,
                   boost::asio::ip::tcp::resolver::results_type results);

    void onConnect(
        boost::beast::error_code ec,
        boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);

    void onSSLHandshake(boost::beast::error_code ec);

    void onHandshake(boost::beast::error_code ec);

    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);

    void onClose(boost::beast::error_code ec);

    void fail(boost::beast::error_code ec, std::string_view op);

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
};

}  // namespace chatterino::eventsub::lib
