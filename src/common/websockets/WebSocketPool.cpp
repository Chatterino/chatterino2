#include "common/websockets/WebSocketPool.hpp"

#include "common/QLogging.hpp"
#include "common/websockets/detail/WebSocketConnectionImpl.hpp"
#include "common/websockets/detail/WebSocketPoolImpl.hpp"

namespace chatterino {

WebSocketPool::WebSocketPool() = default;
WebSocketPool::~WebSocketPool() = default;

WebSocketHandle WebSocketPool::createSocket(
    WebSocketOptions options, std::unique_ptr<WebSocketListener> listener)
{
    if (!this->impl)
    {
        try
        {
            this->impl = std::make_unique<ws::detail::WebSocketPoolImpl>();
        }
        catch (const boost::system::system_error &err)
        {
            // This will only happen if the SSL context failed to be constructed.
            // The user likely runs an incompatible OpenSSL version.
            qCWarning(chatterinoWebsocket)
                << "Failed to create WebSocket implementation" << err.what();
            return {{}};
        }
    }
    if (this->impl->closing)
    {
        return {{}};
    }

    std::shared_ptr<ws::detail::WebSocketConnection> conn;

    if (options.url.scheme() == "wss")
    {
        conn = std::make_shared<ws::detail::TlsWebSocketConnection>(
            std::move(options), this->impl->nextID++, std::move(listener),
            this->impl.get(), this->impl->ioc, this->impl->ssl);
    }
    else if (options.url.scheme() == "ws")
    {
        conn = std::make_shared<ws::detail::TcpWebSocketConnection>(
            std::move(options), this->impl->nextID++, std::move(listener),
            this->impl.get(), this->impl->ioc);
    }
    else
    {
        qCWarning(chatterinoWebsocket) << "Invalid scheme:" << options.url;
        return {{}};
    }

    {
        std::unique_lock guard(this->impl->connectionMutex);
        this->impl->connections.push_back(conn);
    }

    boost::asio::post(this->impl->ioc, [conn] {
        conn->run();
    });

    return {conn};
}

// MARK: WebSocketHandle

WebSocketHandle::WebSocketHandle(
    std::weak_ptr<ws::detail::WebSocketConnection> conn)
    : conn(std::move(conn))
{
}

WebSocketHandle::~WebSocketHandle()
{
    this->close();
}

void WebSocketHandle::close()
{
    auto strong = this->conn.lock();
    if (strong)
    {
        strong->close();
    }
}

void WebSocketHandle::sendText(const QByteArray &data)
{
    auto strong = this->conn.lock();
    if (strong)
    {
        strong->sendText(data);
    }
}

void WebSocketHandle::sendBinary(const QByteArray &data)
{
    auto strong = this->conn.lock();
    if (strong)
    {
        strong->sendBinary(data);
    }
}

}  // namespace chatterino
