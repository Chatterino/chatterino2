#include "singletons/WebSocketPool.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "util/QByteArrayBuffer.hpp"
#include "util/RenameThread.hpp"

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

/// A base class for a WebSocket connection.
///
/// It's agnostic over the backing transport (TCP, TLS, proxy, etc.).
class WebSocketConnection
{
public:
    WebSocketConnection(WebSocketOptions options, int id,
                        std::unique_ptr<WebSocketListener> listener,
                        WebSocketPoolPrivate *parent, asio::io_context &ioc);
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
    std::unique_ptr<WebSocketListener> listener;  // nullable
    WebSocketPoolPrivate *parent;                 // nullable

    asio::ip::tcp::resolver resolver;

    std::deque<std::pair<bool, QByteArrayBuffer>> queuedMessages;
    bool isSending = false;
    int id = 0;

    beast::flat_buffer readBuffer;

    friend QDebug operator<<(QDebug dbg, const WebSocketConnection &conn);
};

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
    using Stream = beast::websocket::stream<Inner>;

    void post(auto &&fn);

    void run() final;
    void close() final;

    void sendText(const QByteArray &data) final;
    void sendBinary(const QByteArray &data) final;

protected:
    Derived *derived();

    void fail(boost::system::error_code ec, QStringView op);
    void doWsHandshake();

    void closeImpl();
    void trySend();

    Stream stream;

private:
    // This is private to ensure only `Derived` can construct this class.
    WebSocketConnectionHelper(WebSocketOptions options, int id,
                              std::unique_ptr<WebSocketListener> listener,
                              WebSocketPoolPrivate *parent,
                              asio::io_context &ioc, Stream stream);

    void onResolve(boost::system::error_code ec,
                   const asio::ip::tcp::resolver::results_type &results);
    void onTcpHandshake(boost::system::error_code ec,
                        const asio::ip::tcp::resolver::endpoint_type &ep);
    void onWsHandshake(boost::system::error_code ec);

    void onReadDone(boost::system::error_code ec, size_t bytesRead);
    void onWriteDone(boost::system::error_code ec, size_t bytesWritten);

    friend Derived;
};

/// A WebSocket connection over TLS (wss://).
class TlsWebSocketConnection
    : public WebSocketConnectionHelper<TlsWebSocketConnection,
                                       asio::ssl::stream<beast::tcp_stream>>
{
public:
    static constexpr int DEFAULT_PORT = 443;

    TlsWebSocketConnection(WebSocketOptions options, int id,
                           std::unique_ptr<WebSocketListener> listener,
                           WebSocketPoolPrivate *parent, asio::io_context &ioc,
                           asio::ssl::context &ssl);

protected:
    bool setupStream(const std::string &host);
    void afterTcpHandshake();

    friend WebSocketConnectionHelper<TlsWebSocketConnection,
                                     asio::ssl::stream<beast::tcp_stream>>;
};

/// A WebSocket connection over TCP (ws://).
class TcpWebSocketConnection
    : public WebSocketConnectionHelper<TcpWebSocketConnection,
                                       beast::tcp_stream>
{
public:
    static constexpr int DEFAULT_PORT = 80;

    TcpWebSocketConnection(WebSocketOptions options, int id,
                           std::unique_ptr<WebSocketListener> listener,
                           WebSocketPoolPrivate *parent, asio::io_context &ioc);

protected:
    void afterTcpHandshake();

    friend WebSocketConnectionHelper<TcpWebSocketConnection, beast::tcp_stream>;
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
    asio::io_context &ioc)
    : options(std::move(options))
    , listener(std::move(listener))
    , parent(parent)
    , resolver(asio::make_strand(ioc))
    , id(id)
{
    qCDebug(chatterinoWebsocket) << *this << "Created";
}

WebSocketConnection::~WebSocketConnection()
{
    assert(!this->listener && !this->parent);
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
    if (this->parent)
    {
        this->parent->removeConnection(this);
        this->parent = nullptr;
    }
    qCDebug(chatterinoWebsocket) << *this << "Detached";
}

// MARK: WebSocketConnectionHelper

template <typename Derived, typename Inner>
WebSocketConnectionHelper<Derived, Inner>::WebSocketConnectionHelper(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolPrivate *parent,
    asio::io_context &ioc, Stream stream)
    : WebSocketConnection(std::move(options), id, std::move(listener), parent,
                          ioc)
    , stream(std::move(stream))
{
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::post(auto &&fn)
{
    asio::post(this->stream.get_executor(), std::forward<decltype(fn)>(fn));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::run()
{
    auto host = this->options.url.host(QUrl::FullyEncoded).toStdString();
    if constexpr (requires { this->derived()->setupStream(host); })
    {
        if (!this->derived()->setupStream(host))
        {
            return;
        }
    }

    this->resolver.async_resolve(
        host, std::to_string(this->options.url.port(Derived::DEFAULT_PORT)),
        beast::bind_front_handler(&WebSocketConnectionHelper::onResolve,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::close()
{
    this->post([self{this->shared_from_this()}] {
        self->closeImpl();
    });
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::sendText(const QByteArray &data)
{
    this->post([self{this->shared_from_this()}, data] {
        self->queuedMessages.emplace_back(true, data);
        self->trySend();
    });
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::sendBinary(
    const QByteArray &data)
{
    this->post([self{this->shared_from_this()}, data] {
        self->queuedMessages.emplace_back(false, data);
        self->trySend();
    });
}

template <typename Derived, typename Inner>
Derived *WebSocketConnectionHelper<Derived, Inner>::derived()
{
    return static_cast<Derived *>(this);
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onResolve(
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
                                    &WebSocketConnectionHelper::onTcpHandshake,
                                    this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onTcpHandshake(
    boost::system::error_code ec,
    const asio::ip::tcp::resolver::endpoint_type &ep)
{
    if (ec)
    {
        this->fail(ec, u"TCP handshake");
        return;
    }

    qCDebug(chatterinoWebsocket) << *this << "TCP handshake done";
    this->options.url.setPort(ep.port());

    this->derived()->afterTcpHandshake();
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::doWsHandshake()
{
    beast::get_lowest_layer(this->stream).expires_never();
    this->stream.set_option(beast::websocket::stream_base::timeout::suggested(
        beast::role_type::client));
    this->stream.set_option(beast::websocket::stream_base::decorator{
        [this](beast::websocket::request_type &req) {
            bool hasUa = false;
            for (const auto &[key, value] : this->options.headers)
            {
                // TODO(Qt 6.5): Use QUtf8StringView
                QLatin1StringView keyView(key.c_str());
                if (QLatin1StringView("user-agent")
                        .compare(keyView, Qt::CaseInsensitive) == 0)
                {
                    hasUa = true;
                }

                try
                {
                    // this can fail if the key or value exceed the maximum size
                    req.set(key, value);
                }
                catch (const boost::system::system_error &err)
                {
                    qCWarning(chatterinoWebsocket)
                        << "Invalid header - name:" << QUtf8StringView(key)
                        << "value:" << QUtf8StringView(value)
                        << "error:" << QUtf8StringView(err.what());
                }
            }

            // default UA
            if (!hasUa)
            {
                auto ua = QStringLiteral("Chatterino/%1 (%2)")
                              .arg(Version::instance().version(),
                                   Version::instance().commitHash())
                              .toStdString();
                req.set(beast::http::field::user_agent, ua);
            }
        },
    });

    auto host = this->options.url.host(QUrl::FullyEncoded).toStdString() + ':' +
                std::to_string(this->options.url.port(Derived::DEFAULT_PORT));
    auto path = this->options.url.path(QUrl::FullyEncoded).toStdString();
    if (path.empty())
    {
        path = "/";
    }
    this->stream.async_handshake(
        host, path,
        beast::bind_front_handler(&WebSocketConnectionHelper::onWsHandshake,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onWsHandshake(
    boost::system::error_code ec)
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
        beast::bind_front_handler(&WebSocketConnectionHelper::onReadDone,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onReadDone(
    boost::system::error_code ec, size_t bytesRead)
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
        beast::bind_front_handler(&WebSocketConnectionHelper::onReadDone,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onWriteDone(
    boost::system::error_code ec, size_t /*bytesWritten*/)
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

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::trySend()
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
        beast::bind_front_handler(&WebSocketConnectionHelper::onWriteDone,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::closeImpl()
{
    qCDebug(chatterinoWebsocket) << *this << "Closing...";
    this->stream.async_close(
        beast::websocket::close_code::normal,
        [this, lifetime{this->shared_from_this()}](auto ec) {
            if (ec)
            {
                qCWarning(chatterinoWebsocket) << *this << "Failed to close"
                                               << QUtf8StringView(ec.message());
            }
            else
            {
                qCDebug(chatterinoWebsocket) << *this << "Closed";
            }
            this->detach();
        });
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::fail(
    boost::system::error_code ec, QStringView op)
{
    qCWarning(chatterinoWebsocket)
        << *this << "Failed:" << op << QUtf8StringView(ec.message());
    if (this->stream.is_open())
    {
        this->closeImpl();
    }
    this->detach();
}

// MARK: TlsWebSocketConnection

TlsWebSocketConnection::TlsWebSocketConnection(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolPrivate *parent,
    asio::io_context &ioc, asio::ssl::context &ssl)
    : WebSocketConnectionHelper(std::move(options), id, std::move(listener),
                                parent, ioc,
                                Stream{asio::make_strand(ioc), ssl})
{
}

bool TlsWebSocketConnection::setupStream(const std::string &host)
{
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (::SSL_set_tlsext_host_name(this->stream.next_layer().native_handle(),
                                   host.c_str()) == 0)
    {
        this->fail({static_cast<int>(::ERR_get_error()),
                    asio::error::get_ssl_category()},
                   u"Setting SNI hostname");
        return false;
    }
    return true;
}

void TlsWebSocketConnection::afterTcpHandshake()
{
    beast::get_lowest_layer(this->stream)
        .expires_after(std::chrono::seconds{30});
    this->stream.next_layer().async_handshake(
        asio::ssl::stream_base::client,
        [this,
         lifetime{this->shared_from_this()}](boost::system::error_code ec) {
            if (ec)
            {
                this->fail(ec, u"TLS handshake");
                return;
            }

            qCDebug(chatterinoWebsocket)
                << *this << "TLS handshake done, using"
                << ::SSL_get_version(this->stream.next_layer().native_handle());
            this->doWsHandshake();
        });
}

// MARK: TcpWebSocketConnection

TcpWebSocketConnection::TcpWebSocketConnection(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolPrivate *parent,
    asio::io_context &ioc)
    : WebSocketConnectionHelper(std::move(options), id, std::move(listener),
                                parent, ioc, Stream{asio::make_strand(ioc)})
{
}

void TcpWebSocketConnection::afterTcpHandshake()
{
    this->doWsHandshake();
}

// MARK: WebSocketPoolPrivate

WebSocketPoolPrivate::WebSocketPoolPrivate()
    : ioc(1)
    , ssl(boost::asio::ssl::context::tls_client)
    , work(this->ioc.get_executor())
{
#ifdef CHATTERINO_WITH_TESTS
    if (!getApp()->isTest())
#endif
    {
        this->ssl.set_verify_mode(
            boost::asio::ssl::verify_peer |
            boost::asio::ssl::verify_fail_if_no_peer_cert);
        this->ssl.set_default_verify_paths();

        boost::certify::enable_native_https_server_verification(this->ssl);
    }

    this->ioThread = std::make_unique<std::thread>([this] {
        this->ioc.run();
    });
    renameThread(*this->ioThread, "WebSocketPool");
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

    std::shared_ptr<WebSocketConnection> conn;

    if (options.url.scheme() == "wss")
    {
        conn = std::make_shared<TlsWebSocketConnection>(
            std::move(options), this->data->nextID++, std::move(listener),
            this->data.get(), this->data->ioc, this->data->ssl);
    }
    else if (options.url.scheme() == "ws")
    {
        conn = std::make_shared<TcpWebSocketConnection>(
            std::move(options), this->data->nextID++, std::move(listener),
            this->data.get(), this->data->ioc);
    }
    else
    {
        qCWarning(chatterinoWebsocket) << "Invalid scheme:" << options.url;
        return {{}};
    }

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
