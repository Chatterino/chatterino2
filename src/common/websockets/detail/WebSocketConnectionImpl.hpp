#pragma once

#include "common/websockets/detail/WebSocketConnection.hpp"

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

namespace chatterino::ws::detail {

/// A CRTP helper to share code between the TLS and TCP connections.
///
/// `Derived` must have a `void afterTcpHandshake()` method, which is called if
/// the TCP handshake was successful. Subclasses can call `doWsHandshake` after
/// the intermediate handshake (i.e. TLS) is done.
///
/// `Derived` must have a constant `DEFAULT_PORT` which specifies the TCP port
/// to connect to if the specified URL doesn't have one set.
///
/// `Derived` can contain a method `bool setupStream(const std::string&)` which
/// is called from `run()`. The return value indicates if an error happened. An
/// implementation must've called `fail()` in case of errors.
template <typename Derived, typename Inner>
class WebSocketConnectionHelper : public WebSocketConnection,
                                  public std::enable_shared_from_this<
                                      WebSocketConnectionHelper<Derived, Inner>>
{
public:
    using Stream = boost::beast::websocket::stream<Inner>;

    void post(auto &&fn);

    void run() final;
    void close() final;

    void sendText(const QByteArray &data) final;
    void sendBinary(const QByteArray &data) final;

protected:
    Derived *derived();

    void fail(boost::system::error_code ec, QStringView op);
    void fail(std::string_view ec, QStringView op);
    void doWsHandshake();

    void closeImpl();
    void trySend();

    Stream stream;

private:
    // This is private to ensure only `Derived` can construct this class.
    WebSocketConnectionHelper(WebSocketOptions options, int id,
                              std::unique_ptr<WebSocketListener> listener,
                              WebSocketPoolImpl *pool,
                              boost::asio::io_context &ioc, Stream stream);

    void onResolve(boost::system::error_code ec,
                   const boost::asio::ip::tcp::resolver::results_type &results);

    /// Initialize a TCP connection to the given endpoint iterator in `resolvedEndpoints`.
    ///
    /// If we failed to connect, try the next iterator.
    ///
    /// If the iterator is invalid, we have run out of endpoints to try, and deem this
    /// connection a failure.
    void tryConnect(boost::asio::ip::tcp::resolver::results_type::const_iterator
                        endpointIterator);
    void onTcpHandshake(
        boost::asio::ip::tcp::resolver::results_type::const_iterator
            endpointIterator,
        boost::system::error_code ec);
    void onWsHandshake(boost::system::error_code ec);

    void onReadDone(boost::system::error_code ec, size_t bytesRead);
    void onWriteDone(boost::system::error_code ec, size_t bytesWritten);

    friend Derived;

    /// A range of endpoints from the `onResolve` function.
    ///
    /// When we successfully resolve the host, we try to connect by
    /// iterating over these results.
    boost::asio::ip::tcp::resolver::results_type resolvedEndpoints;
};

/// A WebSocket connection over TLS (wss://).
class TlsWebSocketConnection
    : public WebSocketConnectionHelper<
          TlsWebSocketConnection,
          boost::asio::ssl::stream<boost::beast::tcp_stream>>
{
public:
    static constexpr int DEFAULT_PORT = 443;

    TlsWebSocketConnection(WebSocketOptions options, int id,
                           std::unique_ptr<WebSocketListener> listener,
                           WebSocketPoolImpl *pool,
                           boost::asio::io_context &ioc,
                           boost::asio::ssl::context &ssl);

protected:
    bool setupStream(const std::string &host);
    void afterTcpHandshake();

    friend WebSocketConnectionHelper<
        TlsWebSocketConnection,
        boost::asio::ssl::stream<boost::beast::tcp_stream>>;
};

/// A WebSocket connection over TCP (ws://).
class TcpWebSocketConnection
    : public WebSocketConnectionHelper<TcpWebSocketConnection,
                                       boost::beast::tcp_stream>
{
public:
    static constexpr int DEFAULT_PORT = 80;

    TcpWebSocketConnection(WebSocketOptions options, int id,
                           std::unique_ptr<WebSocketListener> listener,
                           WebSocketPoolImpl *pool,
                           boost::asio::io_context &ioc);

protected:
    void afterTcpHandshake();

    friend WebSocketConnectionHelper<TcpWebSocketConnection,
                                     boost::beast::tcp_stream>;
};

}  // namespace chatterino::ws::detail
