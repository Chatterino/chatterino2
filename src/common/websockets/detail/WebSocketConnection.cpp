#include "common/websockets/detail/WebSocketConnection.hpp"

#include "common/QLogging.hpp"
#include "WebSocketPoolImpl.hpp"

#include <boost/asio/strand.hpp>

namespace chatterino::ws::detail {

WebSocketConnection::WebSocketConnection(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolImpl *pool,
    boost::asio::io_context &ioc)
    : options(std::move(options))
    , listener(std::move(listener))
    , pool(pool)
    , resolver(boost::asio::make_strand(ioc))
    , id(id)
{
    qCDebug(chatterinoWebsocket) << *this << "Created";
}

WebSocketConnection::~WebSocketConnection()
{
    assert(!this->listener && !this->pool);
    qCDebug(chatterinoWebsocket) << *this << "Destroyed";
}

QDebug operator<<(QDebug dbg, const WebSocketConnection &conn)
{
    QDebugStateSaver state(dbg);

    dbg.noquote().nospace()
        << '[' << conn.id << '|' << conn.options.url.toDisplayString() << ']';

    return dbg;
}

void WebSocketConnection::detach()
{
    if (this->listener)
    {
        this->listener->onClose(std::move(this->listener));
    }
    if (this->pool)
    {
        this->pool->removeConnection(this);
        this->pool = nullptr;
    }
    qCDebug(chatterinoWebsocket) << *this << "Detached";
}

}  // namespace chatterino::ws::detail
