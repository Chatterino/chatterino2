#include "singletons/WebSocketPool.hpp"

#include "common/QLogging.hpp"
#include "util/QByteArrayBuffer.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/certify/https_verification.hpp>
#include <QDebug>

#include <deque>
#include <thread>
#include <utility>


namespace asio = boost::asio;
namespace beast = boost::beast;

namespace chatterino {

class WebSocketConnection
    : public std::enable_shared_from_this<WebSocketConnection>
{
public:
    WebSocketConnection(WebSocketOptions options, int id,
                        std::unique_ptr<WebSocketListener> listener,
                        WebSocketPoolPrivate *parent, asio::io_context &ioc,
                        asio::ssl::context &ssl);
    ~WebSocketConnection();

    WebSocketConnection(const WebSocketConnection &) = delete;
    WebSocketConnection(WebSocketConnection &&) = delete;
    WebSocketConnection &operator=(const WebSocketConnection &) = delete;
    WebSocketConnection &operator=(WebSocketConnection &&) = delete;

    void run();
    void close();

    void sendText(const QByteArray &data);
    void sendBinary(const QByteArray &data);

    void post(auto &&fn);

private:
    void fail(boost::system::error_code ec, QStringView op);

    void closeImpl();
    void detach();

    void onResolve(boost::system::error_code ec,
                   const asio::ip::tcp::resolver::results_type &results);
    void onTcpHandshake(boost::system::error_code ec,
                        const asio::ip::tcp::resolver::endpoint_type &ep);
    void onTlsHandshake(boost::system::error_code ec);
    void onWsHandshake(boost::system::error_code ec);

    void onReadDone(boost::system::error_code ec, size_t bytesRead);
    void onWriteDone(boost::system::error_code ec, size_t bytesWritten);

    void trySend();

    WebSocketOptions options;
    std::unique_ptr<WebSocketListener> listener;  // nullable
    WebSocketPoolPrivate *parent;                 // nullable

    beast::websocket::stream<asio::ssl::stream<beast::tcp_stream>> stream;
    asio::ip::tcp::resolver resolver;

    std::deque<std::pair<bool, QByteArrayBuffer>> queuedMessages;
    bool isSending = false;
    int id = 0;

    beast::flat_buffer readBuffer;

    friend QDebug operator<<(QDebug dbg, const WebSocketConnection &conn);
};

class WebSocketPoolPrivate
{
public:
    WebSocketPoolPrivate();
    ~WebSocketPoolPrivate();

    WebSocketPoolPrivate(const WebSocketPoolPrivate &) = delete;
    WebSocketPoolPrivate(WebSocketPoolPrivate &&) = delete;
    WebSocketPoolPrivate &operator=(const WebSocketPoolPrivate &) = delete;
    WebSocketPoolPrivate &operator=(WebSocketPoolPrivate &&) = delete;

    void removeConnection(WebSocketConnection *conn);

    std::unique_ptr<std::thread> ioThread;
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::shared_ptr<WebSocketConnection>> connections;
    std::mutex connectionMutex;

    bool closing = false;
    int nextID = 1;
};

// MARK: WebSocketConnection

WebSocketConnection::WebSocketConnection(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolPrivate *parent,
    asio::io_context &ioc, asio::ssl::context &ssl)
    : options(std::move(options))
    , listener(std::move(listener))
    , parent(parent)
    , stream(asio::make_strand(ioc), ssl)
    , resolver(asio::make_strand(ioc))
    , id(id)
{
    qCDebug(chatterinoWebsocket) << *this << "Created";
}

WebSocketConnection::~WebSocketConnection()
{
    qCDebug(chatterinoWebsocket) << *this << "Destroyed";
}

QDebug operator<<(QDebug dbg, const WebSocketConnection &conn)
{
    QDebugStateSaver state(dbg);

    dbg.noquote().nospace()
        << '[' << conn.id << '|' << conn.options.url.toDisplayString() << ']';

    return dbg;
}

void WebSocketConnection::run()
{
    auto host = this->options.url.host(QUrl::FullyEncoded).toStdString();
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (::SSL_set_tlsext_host_name(this->stream.next_layer().native_handle(),
                                   host.c_str()) == 0)
    {
        this->fail({static_cast<int>(::ERR_get_error()),
                    asio::error::get_ssl_category()},
                   u"Setting SNI hostname");
        return;
    }

    this->resolver.async_resolve(
        host, std::to_string(this->options.url.port(443)),
        beast::bind_front_handler(&WebSocketConnection::onResolve,
                                  this->shared_from_this()));
}

void WebSocketConnection::close()
{
    this->post([self{this->shared_from_this()}] {
        self->closeImpl();
    });
}

void WebSocketConnection::sendText(const QByteArray &data)
{
    this->post([self{this->shared_from_this()}, data] {
        self->queuedMessages.emplace_back(true, data);
        self->trySend();
    });
}

void WebSocketConnection::sendBinary(const QByteArray &data)
{
    this->post([self{this->shared_from_this()}, data] {
        self->queuedMessages.emplace_back(false, data);
        self->trySend();
    });
}

void WebSocketConnection::onResolve(
    boost::system::error_code ec,
    const asio::ip::tcp::resolver::results_type &results)
{
    if (ec)
    {
        this->fail(ec, u"resolve");
        return;
    }

    qCDebug(chatterinoWebsocket) << *this << "Resolved host";

    beast::get_lowest_layer(this->stream)
        .expires_after(std::chrono::seconds{30});
    beast::get_lowest_layer(this->stream)
        .async_connect(results, beast::bind_front_handler(
                                    &WebSocketConnection::onTcpHandshake,
                                    this->shared_from_this()));
}

void WebSocketConnection::onTcpHandshake(
    boost::system::error_code ec,
    const asio::ip::tcp::resolver::endpoint_type &ep)
{
    if (ec)
    {
        this->fail(ec, u"TCP handshake");
        return;
    }

    qCDebug(chatterinoWebsocket) << *this << "TCP handshake done";

    beast::get_lowest_layer(this->stream)
        .expires_after(std::chrono::seconds{30});
    this->options.url.setPort(ep.port());
    this->stream.next_layer().async_handshake(
        asio::ssl::stream_base::client,
        beast::bind_front_handler(&WebSocketConnection::onTlsHandshake,
                                  this->shared_from_this()));
}

void WebSocketConnection::onTlsHandshake(boost::system::error_code ec)
{
    if (ec)
    {
        this->fail(ec, u"TLS handshake");
        return;
    }

    qCDebug(chatterinoWebsocket)
        << *this << "TLS handshake done, using"
        << ::SSL_get_version(this->stream.next_layer().native_handle());

    beast::get_lowest_layer(this->stream).expires_never();
    this->stream.set_option(beast::websocket::stream_base::timeout::suggested(
        beast::role_type::client));
    this->stream.set_option(beast::websocket::stream_base::decorator{
        [](beast::websocket::request_type &req) {
            req.set(beast::http::field::user_agent, "Chatterino TODO");
        },
    });

    auto host = this->options.url.host(QUrl::FullyEncoded).toStdString() + ':' +
                std::to_string(this->options.url.port(443));
    auto path = this->options.url.path(QUrl::FullyEncoded).toStdString();
    if (path.empty())
    {
        path = "/";
    }
    this->stream.async_handshake(
        host, path,
        beast::bind_front_handler(&WebSocketConnection::onWsHandshake,
                                  this->shared_from_this()));
}

void WebSocketConnection::onWsHandshake(boost::system::error_code ec)
{
    if (ec)
    {
        this->fail(ec, u"WS handshake");
        return;
    }

    qCDebug(chatterinoWebsocket) << *this << "WS handshake done";

    this->trySend();
    this->stream.async_read(
        this->readBuffer,
        beast::bind_front_handler(&WebSocketConnection::onReadDone,
                                  this->shared_from_this()));
}

void WebSocketConnection::onReadDone(boost::system::error_code ec,
                                     size_t bytesRead)
{
    if (!this->listener)
    {
        return;
    }
    if (ec)
    {
        this->fail(ec, u"read");
        return;
    }

    // XXX: this copies - we could read directly into a QByteArray
    QByteArray data{
        static_cast<const char *>(this->readBuffer.cdata().data()),
        static_cast<QByteArray::size_type>(bytesRead),
    };
    this->readBuffer.consume(bytesRead);

    if (this->stream.got_text())
    {
        this->listener->onTextMessage(std::move(data));
    }
    else
    {
        this->listener->onBinaryMessage(std::move(data));
    }

    this->stream.async_read(
        this->readBuffer,
        beast::bind_front_handler(&WebSocketConnection::onReadDone,
                                  this->shared_from_this()));
}

void WebSocketConnection::onWriteDone(boost::system::error_code ec,
                                      size_t /*bytesWritten*/)
{
    if (!this->queuedMessages.empty())
    {
        this->queuedMessages.pop_front();
    }
    else
    {
        assert(false);
    }
    this->isSending = false;

    if (ec)
    {
        this->fail(ec, u"write");
        return;
    }

    this->trySend();
}

void WebSocketConnection::trySend()
{
    if (this->queuedMessages.empty() || this->isSending ||
        !this->stream.is_open())
    {
        return;
    }

    this->isSending = true;
    this->stream.text(this->queuedMessages.front().first);
    this->stream.async_write(
        this->queuedMessages.front().second,
        beast::bind_front_handler(&WebSocketConnection::onWriteDone,
                                  this->shared_from_this()));
}

void WebSocketConnection::closeImpl()
{
    qCDebug(chatterinoWebsocket) << *this << "Closing...";
    this->stream.async_close(
        beast::websocket::close_code::normal,
        [self{this->shared_from_this()}](auto ec) {
            if (ec)
            {
                qCWarning(chatterinoWebsocket)
                    << *self << "Failed to close" << ec.message();
            }
            else
            {
                qCDebug(chatterinoWebsocket) << *self << "Closed";
            }
            self->detach();
        });
}

void WebSocketConnection::post(auto &&fn)
{
    asio::post(this->stream.get_executor(), std::forward<decltype(fn)>(fn));
}

void WebSocketConnection::fail(boost::system::error_code ec, QStringView op)
{
    qCWarning(chatterinoWebsocket) << *this << "Failed:" << op << ec.message();
    if (this->stream.is_open())
    {
        this->closeImpl();
    }
    this->detach();
}

void WebSocketConnection::detach()
{
    if (this->listener)
    {
        this->listener->onClose(std::move(this->listener));
    }
    if (this->parent)
    {
        this->parent->removeConnection(this);
        this->parent = nullptr;
    }
    qCDebug(chatterinoWebsocket) << *this << "Detached";
}

// MARK: WebSocketPoolPrivate

WebSocketPoolPrivate::WebSocketPoolPrivate()
    : ioc(1)
    , ssl(boost::asio::ssl::context::tls_client)
    , work(this->ioc.get_executor())
{
    this->ssl.set_verify_mode(boost::asio::ssl::verify_peer |
                              boost::asio::ssl::verify_fail_if_no_peer_cert);
    this->ssl.set_default_verify_paths();

    boost::certify::enable_native_https_server_verification(this->ssl);

    this->ioThread = std::make_unique<std::thread>([this] {
        this->ioc.run();
    });
}

WebSocketPoolPrivate::~WebSocketPoolPrivate()
{
    this->closing = true;
    this->work.reset();
    {
        std::lock_guard g(this->connectionMutex);
        for (const auto &conn : this->connections)
        {
            conn->close();
        }
    }
    if (!this->ioThread)
    {
        return;
    }
    if (this->ioThread->joinable())
    {
        this->ioThread->join();
    }
}

void WebSocketPoolPrivate::removeConnection(WebSocketConnection *conn)
{
    std::lock_guard g(this->connectionMutex);
    std::erase_if(this->connections, [conn](const auto &v) {
        return v.get() == conn;
    });
}

// MARK: WebSocketPool

WebSocketPool::WebSocketPool() = default;
WebSocketPool::~WebSocketPool() = default;

WebSocketHandle WebSocketPool::createSocket(
    WebSocketOptions options, std::unique_ptr<WebSocketListener> listener)
{
    if (!this->data)
    {
        this->data = std::make_unique<WebSocketPoolPrivate>();
    }
    if (this->data->closing)
    {
        return {{}};
    }

    auto conn = std::make_shared<WebSocketConnection>(
        std::move(options), this->data->nextID++, std::move(listener),
        this->data.get(), this->data->ioc, this->data->ssl);

    {
        std::unique_lock guard(this->data->connectionMutex);
        this->data->connections.push_back(conn);
    }

    boost::asio::post(this->data->ioc, [conn] {
        conn->run();
    });

    return {conn};
}

// MARK: WebSocketHandle

WebSocketHandle::WebSocketHandle(std::weak_ptr<WebSocketConnection> conn)
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
