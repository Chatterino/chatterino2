#include "providers/ws/Client.hpp"

#include "common/Literals.hpp"
#include "common/Version.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"

#include <boost/asio/executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

#include <memory>
#include <optional>
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
public:
    ClientPrivate(Client *owner)
        : owner_(owner)
    {
        this->websocketClient.set_access_channels(
            websocketpp::log::alevel::all);
        this->websocketClient.clear_access_channels(
            websocketpp::log::alevel::frame_payload |
            websocketpp::log::alevel::frame_header);

        this->websocketClient.init_asio();

        this->websocketClient.set_tls_init_handler([](auto) {
            return ClientPrivate::onTLSInit();
        });

        this->websocketClient.set_message_handler([this](auto hdl, auto msg) {
            this->onMessage(std::move(hdl), msg);
        });
        this->websocketClient.set_open_handler([this](auto hdl) {
            this->onConnectionOpen(std::move(hdl));
        });
        this->websocketClient.set_close_handler([this](auto hdl) {
            this->onConnectionClose(std::move(hdl));
        });
        this->websocketClient.set_fail_handler([this](auto hdl) {
            this->onConnectionFail(std::move(hdl));
        });
        this->websocketClient.set_user_agent(
            u"Chatterino/%1 (%2)"_s
                .arg(Version::instance().version(),
                     Version::instance().commitHash())
                .toStdString());

        this->work.emplace(boost::asio::make_work_guard(
            this->websocketClient.get_io_service()));
    }

    static WebsocketppContextPtr onTLSInit();
    void onMessage(WebsocketppHandle &&hdl, const WebsocketppMessagePtr &msg);
    void onConnectionOpen(WebsocketppHandle &&hdl);
    void onConnectionClose(WebsocketppHandle &&hdl);
    void onConnectionFail(WebsocketppHandle &&hdl);

    void runThread();

    WebsocketppClient websocketClient;
    std::unique_ptr<std::thread> asioThread;

    std::optional<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work;

private:
    Client *owner_;
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
    // TODO(Qt6): this should be a QByteArrayView
    this->owner_->onTextMessage(
        std::move(hdl),
        QLatin1String(msg->get_payload().c_str(),
                      static_cast<qsizetype>(msg->get_payload().size())));
}

void ClientPrivate::onConnectionOpen(WebsocketppHandle &&hdl)
{
    this->owner_->onConnectionOpen(std::move(hdl));
}

void ClientPrivate::onConnectionClose(WebsocketppHandle &&hdl)
{
    this->owner_->onConnectionClosed(std::move(hdl));
}

void ClientPrivate::onConnectionFail(WebsocketppHandle &&hdl)
{
    websocketpp::lib::error_code ec;
    auto conn = this->websocketClient.get_con_from_hdl(std::move(hdl));
    if (ec)
    {
        this->owner_->onConnectionFailed("<failed to get connection back>"_L1);
        return;
    }

    auto msg = conn->get_ec().message();
    this->owner_->onConnectionFailed(
        QLatin1String(msg.c_str(), static_cast<qsizetype>(msg.size())));
}

void ClientPrivate::runThread()
{
    qCDebug(chatterinoWebsocket) << "Start WebSocket manager thread";
    this->websocketClient.run();
    qCDebug(chatterinoWebsocket) << "Done with WebSocket manager thread";
}

// ====== Client ======

Client::Client()
    : private_(new ClientPrivate(this))
{
}

Client::~Client() = default;

void Client::start()
{
    auto *d = this->private_.get();

    d->asioThread = std::make_unique<std::thread>([d] {
        d->runThread();
    });
}

void Client::stop()
{
    auto *d = this->private_.get();

    if (d->work)
    {
        d->work->reset();
    }

    if (d->asioThread->joinable())
    {
        d->asioThread->join();
    }
}

void Client::addConnection(const QString &host)
{
    auto *d = this->private_.get();

    websocketpp::lib::error_code ec;
    auto con = d->websocketClient.get_connection(host.toStdString(), ec);

    if (ec)
    {
        qCDebug(chatterinoWebsocket)
            << "Unable to establish connection:" << ec.message().c_str();
        return;
    }

    NetworkConfigurationProvider::applyToWebSocket(con);

    d->websocketClient.connect(con);
}

bool Client::sendText(const Connection &conn, const char *str, size_t len)
{
    auto *d = this->private_.get();

    WebsocketppErrorCode ec;
    d->websocketClient.send(conn.hdl_, str, len,
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
    auto *d = this->private_.get();

    WebsocketppErrorCode ec;

    auto conn = d->websocketClient.get_con_from_hdl(weakConn.hdl_, ec);
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
    auto *d = this->private_.get();

    auto timer = std::make_shared<boost::asio::steady_timer>(
        d->websocketClient.get_io_service());
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
