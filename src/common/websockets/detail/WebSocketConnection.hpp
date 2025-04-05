#pragma once

#include "common/websockets/WebSocketPool.hpp"
#include "util/QByteArrayBuffer.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <QDebug>

#include <deque>
#include <memory>
#include <utility>

namespace chatterino::ws::detail {

class WebSocketPoolImpl;

/// A base class for a WebSocket connection.
///
/// It's agnostic over the backing transport (TCP, TLS, proxy, etc.).
class WebSocketConnection
{
public:
    WebSocketConnection(WebSocketOptions options, int id,
                        std::unique_ptr<WebSocketListener> listener,
                        WebSocketPoolImpl *pool, boost::asio::io_context &ioc);
    virtual ~WebSocketConnection();

    WebSocketConnection(const WebSocketConnection &) = delete;
    WebSocketConnection(WebSocketConnection &&) = delete;
    WebSocketConnection &operator=(const WebSocketConnection &) = delete;
    WebSocketConnection &operator=(WebSocketConnection &&) = delete;

    /// Start connecting.
    ///
    /// Must be called from the desired executor.
    virtual void run() = 0;

    /// Close this connection gracefully (if possible).
    ///
    /// Can be called from any thread.
    virtual void close() = 0;

    /// Send or queue a text message.
    ///
    /// Can be called from any thread.
    virtual void sendText(const QByteArray &data) = 0;

    /// Send or queue a binary message.
    ///
    /// Can be called from any thread.
    virtual void sendBinary(const QByteArray &data) = 0;

protected:
    /// Reset and notify the parent and listener (if possible).
    ///
    /// - If the listener is set, notify it about a close event.
    /// - If the parent is set, notify it about a closed connection.
    /// - Set the listener and parent to (the equivalent of) nullptr.
    void detach();

    WebSocketOptions options;
    // nullable, used for signalling a disconnect
    std::unique_ptr<WebSocketListener> listener;
    // nullable, used for signalling a disconnect
    WebSocketPoolImpl *pool;

    boost::asio::ip::tcp::resolver resolver;

    std::deque<std::pair<bool, QByteArrayBuffer>> queuedMessages;
    bool isSending = false;
    bool isClosing = false;
    int id = 0;

    boost::beast::flat_buffer readBuffer;

    friend QDebug operator<<(QDebug dbg, const WebSocketConnection &conn);
};

}  // namespace chatterino::ws::detail
