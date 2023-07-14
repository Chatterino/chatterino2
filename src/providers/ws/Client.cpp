#include "providers/ws/Client.hpp"

#include "common/Literals.hpp"
#include "common/Version.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"

#include <websocketpp/client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

#include <memory>
#include <utility>

namespace {

struct WebsocketConfig : public websocketpp::config::asio_tls_client {
    using elog_type =
        websocketpp::log::chatterinowebsocketpplogger<concurrency_type,
                                                      websocketpp::log::elevel>;
    using alog_type =
        websocketpp::log::chatterinowebsocketpplogger<concurrency_type,
                                                      websocketpp::log::alevel>;

    struct PerMessageDeflateConfig {
    };

    using permessage_deflate_type =
        websocketpp::extensions::permessage_deflate::disabled<
            PerMessageDeflateConfig>;
};

using WebsocketppClient = websocketpp::client<WebsocketConfig>;
using WebsocketppHandle = websocketpp::connection_hdl;
using WebsocketppErrorCode = websocketpp::lib::error_code;
using WebsocketppMessagePtr =
    websocketpp::config::asio_tls_client::message_type::ptr;
using WebsocketppContextPtr =
    websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

}  // namespace

namespace chatterino::ws {

using namespace literals;

class ClientPrivate
{
    ClientPrivate(Client *owner)
        : q_ptr(owner)
    {
        this->websocketClient_.set_access_channels(
            websocketpp::log::alevel::all);
        this->websocketClient_.clear_access_channels(
            websocketpp::log::alevel::frame_payload |
            websocketpp::log::alevel::frame_header);

        this->websocketClient_.init_asio();

        this->websocketClient_.set_tls_init_handler([](auto) {
            return ClientPrivate::onTLSInit();
        });

        this->websocketClient_.set_message_handler([this](auto hdl, auto msg) {
            this->onMessage(std::move(hdl), msg);
        });
        this->websocketClient_.set_open_handler([this](auto hdl) {
            this->onConnectionOpen(std::move(hdl));
        });
        this->websocketClient_.set_close_handler([this](auto hdl) {
            this->onConnectionClose(std::move(hdl));
        });
        this->websocketClient_.set_fail_handler([this](auto hdl) {
            this->onConnectionFail(std::move(hdl));
        });
        this->websocketClient_.set_user_agent(
            u"Chatterino/%1 (%2)"_s
                .arg(Version::instance().version(),
                     Version::instance().commitHash())
                .toStdString());
    }

    static WebsocketppContextPtr onTLSInit();
    void onMessage(WebsocketppHandle &&hdl, const WebsocketppMessagePtr &msg);
    void onConnectionOpen(WebsocketppHandle &&hdl);
    void onConnectionClose(WebsocketppHandle &&hdl);
    void onConnectionFail(WebsocketppHandle &&hdl);

    void runThread();

    Client *q_ptr;
    Q_DECLARE_PUBLIC(Client)

    std::shared_ptr<boost::asio::io_service::work> work_{nullptr};

    WebsocketppClient websocketClient_;
    std::unique_ptr<std::thread> asioThread_;
};

WebsocketppContextPtr ClientPrivate::onTLSInit()
{
    WebsocketppContextPtr ctx(
        new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    }
    catch (const std::exception &e)
    {
        qCDebug(chatterinoWebsocket)
            << "Exception caught in onTLSInit:" << e.what();
    }

    return ctx;
}

void ClientPrivate::onMessage(WebsocketppHandle &&hdl,
                              const WebsocketppMessagePtr &msg)
{
    Q_Q(Client);

    // TODO(Qt6): this should be a QByteArrayView
    q->onTextMessage(
        std::move(hdl),
        QLatin1String(msg->get_payload().c_str(),
                      static_cast<qsizetype>(msg->get_payload().size())));
}

void ClientPrivate::onConnectionOpen(WebsocketppHandle &&hdl)
{
    Q_Q(Client);

    q->onConnectionOpen(std::move(hdl));
}

void ClientPrivate::onConnectionClose(WebsocketppHandle &&hdl)
{
    Q_Q(Client);

    q->onConnectionClosed(std::move(hdl));
}

void ClientPrivate::onConnectionFail(WebsocketppHandle &&hdl)
{
    Q_Q(Client);

    websocketpp::lib::error_code ec;
    auto conn = this->websocketClient_.get_con_from_hdl(std::move(hdl));
    if (ec)
    {
        q->onConnectionFailed("<failed to get connection back>"_L1);
        return;
    }

    auto msg = conn->get_ec().message();
    q->onConnectionFailed(
        QLatin1String(msg.c_str(), static_cast<qsizetype>(msg.size())));
}

void ClientPrivate::runThread()
{
    qCDebug(chatterinoWebsocket) << "Start WebSocket manager thread";
    this->websocketClient_.run();
    qCDebug(chatterinoWebsocket) << "Done with WebSocket manager thread";
}

// ====== Client ======

Client::Client()
    : d_ptr(new ClientPrivate(this))
{
}

Client::~Client() = default;

void Client::start()
{
    Q_D(Client);

    d->work_ = std::make_shared<boost::asio::io_service::work>(
        d->websocketClient_.get_io_service());
    d->asioThread_ = std::make_unique<std::thread>([d] {
        d->runThread();
    });
}

void Client::stop()
{
    Q_D(Client);

    d->work_.reset();

    if (d->asioThread_->joinable())
    {
        d->asioThread_->join();
    }
}

void Client::addConnection(const QString &host)
{
    Q_D(Client);

    websocketpp::lib::error_code ec;
    auto con = d->websocketClient_.get_connection(host.toStdString(), ec);

    if (ec)
    {
        qCDebug(chatterinoWebsocket)
            << "Unable to establish connection:" << ec.message().c_str();
        return;
    }

    NetworkConfigurationProvider::applyToWebSocket(con);

    d->websocketClient_.connect(con);
}

bool Client::sendText(const Connection &conn, const char *str, size_t len)
{
    Q_D(Client);

    WebsocketppErrorCode ec;
    d->websocketClient_.send(conn.hdl_, str, len,
                             websocketpp::frame::opcode::text, ec);

    if (ec)
    {
        qCDebug(chatterinoLiveupdates)
            << "Error sending message"
            << QLatin1String(str, static_cast<qsizetype>(len)) << ":"
            << ec.message().c_str();
        return false;
    }

    return true;
}

bool Client::sendText(const Connection &conn, const QByteArray &buf)
{
    return this->sendText(conn, buf.data(), buf.length());
}

bool Client::sendText(const Connection &conn, const QLatin1String &str)
{
    return this->sendText(conn, str.data(), str.size());
}

void Client::close(const Connection &weakConn, const QString &reason,
                   CloseCode code)
{
    Q_D(Client);

    WebsocketppErrorCode ec;

    auto conn = d->websocketClient_.get_con_from_hdl(weakConn.hdl_, ec);
    if (ec)
    {
        qCDebug(chatterinoLiveupdates)
            << "Error getting connection:" << ec.message().c_str();
        return;
    }

    conn->close(static_cast<uint16_t>(code), reason.toStdString(), ec);
    if (ec)
    {
        qCDebug(chatterinoLiveupdates)
            << "Error closing:" << ec.message().c_str();
        return;
    }
}

void Client::runAfter(std::chrono::milliseconds duration,
                      const std::function<void()> &fn)
{
    Q_D(Client);

    auto timer = std::make_shared<boost::asio::steady_timer>(
        d->websocketClient_.get_io_service());
    timer->expires_from_now(duration);

    timer->async_wait([timer, fn](const boost::system::error_code &ec) {
        if (ec)
        {
            qCDebug(chatterinoWebsocket)
                << "Error in runAfter:" << ec.message().c_str();
            return;
        }

        fn();
    });
}

}  // namespace chatterino::ws
