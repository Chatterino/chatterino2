#include "common/websockets/detail/WebSocketPoolImpl.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/websockets/detail/WebSocketConnection.hpp"
#include "util/RenameThread.hpp"

#include <boost/certify/https_verification.hpp>

namespace chatterino::ws::detail {

WebSocketPoolImpl::WebSocketPoolImpl()
    : ioc(1)
    , ssl(boost::asio::ssl::context::tls_client)
    , work(this->ioc.get_executor())
{
    boost::system::error_code ec;
    auto _ = this->ssl.set_options(
        boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::single_dh_use,
        ec);
    if (ec)
    {
        qCWarning(chatterinoWebsocket) << "Failed to set SSL context options"
                                       << QString::fromStdString(ec.message());
    }

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
        this->stoppedFlag.set();
    });
    renameThread(*this->ioThread, "WebSocketPool");
}

WebSocketPoolImpl::~WebSocketPoolImpl()
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

    // Note:
    if (this->stoppedFlag.waitFor(std::chrono::milliseconds{1000}))
    {
        this->ioThread->join();
        return;
    }

    qCWarning(chatterinoWebsocket)
        << "IO-Thread didn't finish after stopping, discard it";
    // detach the thread so the destructor doesn't attempt any joining
    this->ioThread->detach();
}

void WebSocketPoolImpl::removeConnection(WebSocketConnection *conn)
{
    std::lock_guard g(this->connectionMutex);
    std::erase_if(this->connections, [conn](const auto &v) {
        return v.get() == conn;
    });
}

}  // namespace chatterino::ws::detail
