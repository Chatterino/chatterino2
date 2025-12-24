#include "common/websockets/detail/WebSocketConnectionImpl.hpp"

#include "common/QLogging.hpp"
#include "common/Version.hpp"

#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/websocket/ssl.hpp>

using namespace std::literals::string_view_literals;

namespace chatterino::ws::detail {

namespace asio = boost::asio;
namespace beast = boost::beast;

// MARK: WebSocketConnectionHelper

template <typename Derived, typename Inner>
WebSocketConnectionHelper<Derived, Inner>::WebSocketConnectionHelper(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolImpl *pool,
    asio::io_context &ioc, Stream stream)
    : WebSocketConnection(std::move(options), id, std::move(listener), pool,
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

    this->resolvedEndpoints = results;

    this->tryConnect(this->resolvedEndpoints.begin());
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::tryConnect(
    boost::asio::ip::tcp::resolver::results_type::const_iterator
        endpointIterator)
{
    if (endpointIterator == this->resolvedEndpoints.end())
    {
        this->fail("Ran out of resolved endpoints"sv, u"connect");
        return;
    }

    const auto &endpoint = endpointIterator->endpoint();

    qCDebug(chatterinoWebsocket)
        << *this << "connect to" << endpoint.address().to_string();

    beast::get_lowest_layer(this->stream)
        .expires_after(std::chrono::seconds{30});

    beast::get_lowest_layer(this->stream)
        .async_connect(endpoint,
                       beast::bind_front_handler(
                           &WebSocketConnectionHelper::onTcpHandshake,
                           this->shared_from_this(), endpointIterator));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onTcpHandshake(
    boost::asio::ip::tcp::resolver::results_type::const_iterator
        endpointIterator,
    boost::system::error_code ec)
{
    const auto &ep = endpointIterator->endpoint();

    if (ec)
    {
        qCDebug(chatterinoWebsocket)
            << *this << "error in tcp handshake" << ep.address().to_string()
            << ec.message();
        this->tryConnect(++endpointIterator);
        return;
    }

    qCDebug(chatterinoWebsocket)
        << *this << "TCP handshake done" << ep.address().to_string();
    this->options.url.setPort(ep.port());

    // We are done with the endpoints, we can clear the range.
    this->resolvedEndpoints = {};

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
    auto path = this->options.url.path(QUrl::FullyEncoded);
    if (path.isEmpty())
    {
        path = "/";
    }
    if (this->options.url.hasQuery())
    {
        path += '?';
        path += this->options.url.query(QUrl::FullyEncoded);
    }
    this->stream.async_handshake(
        host, path.toStdString(),
        beast::bind_front_handler(&WebSocketConnectionHelper::onWsHandshake,
                                  this->shared_from_this()));
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::onWsHandshake(
    boost::system::error_code ec)
{
    if (!this->listener || this->isClosing)
    {
        return;
    }
    if (ec)
    {
        this->fail(ec, u"WS handshake");
        return;
    }

    qCDebug(chatterinoWebsocket) << *this << "WS handshake done";

    this->listener->onOpen();
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
    if (!this->listener || this->isClosing)
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
    if (this->isClosing)
    {
        return;
    }
    this->isClosing = true;

    qCDebug(chatterinoWebsocket) << *this << "Closing...";

    // cancel all pending operations
    this->resolver.cancel();
    beast::get_lowest_layer(this->stream).cancel();

    this->stream.async_close(
        beast::websocket::close_code::normal,
        [this, lifetime{this->shared_from_this()}](auto ec) {
            if (ec)
            {
                qCWarning(chatterinoWebsocket) << *this << "Failed to close"
                                               << QUtf8StringView(ec.message());
                // make sure we cancel all operations
                beast::get_lowest_layer(this->stream).close();
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
    this->fail(ec.message(), op);
}

template <typename Derived, typename Inner>
void WebSocketConnectionHelper<Derived, Inner>::fail(std::string_view ec,
                                                     QStringView op)
{
    qCWarning(chatterinoWebsocket)
        << *this << "Failed:" << op << QUtf8StringView(ec);
    if (this->stream.is_open())
    {
        this->closeImpl();
    }
    this->detach();
}

// MARK: TlsWebSocketConnection

TlsWebSocketConnection::TlsWebSocketConnection(
    WebSocketOptions options, int id,
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolImpl *pool,
    asio::io_context &ioc, asio::ssl::context &ssl)
    : WebSocketConnectionHelper(std::move(options), id, std::move(listener),
                                pool, ioc, Stream{asio::make_strand(ioc), ssl})
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
    std::unique_ptr<WebSocketListener> listener, WebSocketPoolImpl *pool,
    asio::io_context &ioc)
    : WebSocketConnectionHelper(std::move(options), id, std::move(listener),
                                pool, ioc, Stream{asio::make_strand(ioc)})
{
}

void TcpWebSocketConnection::afterTcpHandshake()
{
    this->doWsHandshake();
}

}  // namespace chatterino::ws::detail
