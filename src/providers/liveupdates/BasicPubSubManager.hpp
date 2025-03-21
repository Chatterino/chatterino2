#pragma once

#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/liveupdates/BasicPubSubWebsocket.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/PubSubHelpers.hpp"
#include "util/DebugCount.hpp"
#include "util/ExponentialBackoff.hpp"
#include "util/OnceFlag.hpp"
#include "util/RenameThread.hpp"

#include <pajlada/signals/signal.hpp>
#include <QJsonObject>
#include <QScopeGuard>
#include <QString>
#include <QStringBuilder>
#include <websocketpp/client.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace chatterino {

/**
 * This class is the basis for connecting and interacting with
 * simple PubSub servers over the Websocket protocol.
 * It acts as a pool for connections (see BasicPubSubClient).
 *
 * You can customize the clients, by creating your custom
 * client in ::createClient.
 *
 * You **must** implement #onMessage. The method gets called for every
 * received message on every connection.
 * If you want to get the connection this message was received on,
 * use #findClient.
 *
 * You must expose your own subscribe and unsubscribe methods
 * (e.g. [un-]subscribeTopic).
 * This manager does not keep track of the subscriptions.
 *
 * @tparam Subscription
 * The subscription has the following requirements:
 * It must have the methods QByteArray encodeSubscribe(),
 * and QByteArray encodeUnsubscribe().
 * It must have an overload for
 * QDebug &operator<< (see tests/src/BasicPubSub.cpp),
 * a specialization for std::hash,
 * and and overload for operator== and operator!=.
 *
 * @see BasicPubSubClient
 */
template <typename Subscription>
class BasicPubSubManager
{
public:
    BasicPubSubManager(QString host, QString shortName)
        : host_(std::move(host))
        , shortName_(std::move(shortName))
    {
        this->websocketClient_.set_access_channels(
            websocketpp::log::alevel::all);
        this->websocketClient_.clear_access_channels(
            websocketpp::log::alevel::frame_payload |
            websocketpp::log::alevel::frame_header);

        this->websocketClient_.init_asio();

        // SSL Handshake
        this->websocketClient_.set_tls_init_handler([this](auto hdl) {
            return this->onTLSInit(hdl);
        });

        this->websocketClient_.set_message_handler([this](auto hdl, auto msg) {
            this->onMessage(hdl, msg);
        });
        this->websocketClient_.set_open_handler([this](auto hdl) {
            this->onConnectionOpen(hdl);
        });
        this->websocketClient_.set_close_handler([this](auto hdl) {
            this->onConnectionClose(hdl);
        });
        this->websocketClient_.set_fail_handler([this](auto hdl) {
            this->onConnectionFail(hdl);
        });
        this->websocketClient_.set_user_agent(
            QStringLiteral("Chatterino/%1 (%2)")
                .arg(Version::instance().version(),
                     Version::instance().commitHash())
                .toStdString());
    }

    virtual ~BasicPubSubManager()
    {
        // The derived class must call stop in its destructor
        assert(this->stopping_);
    }

    BasicPubSubManager(const BasicPubSubManager &) = delete;
    BasicPubSubManager(const BasicPubSubManager &&) = delete;
    BasicPubSubManager &operator=(const BasicPubSubManager &) = delete;
    BasicPubSubManager &operator=(const BasicPubSubManager &&) = delete;

    /** This is only used for testing. */
    struct {
        std::atomic<uint32_t> connectionsClosed{0};
        std::atomic<uint32_t> connectionsOpened{0};
        std::atomic<uint32_t> connectionsFailed{0};
    } diag;

    void start()
    {
        this->work_ = std::make_shared<boost::asio::executor_work_guard<
            boost::asio::io_context::executor_type>>(
            this->websocketClient_.get_io_service().get_executor());
        this->mainThread_.reset(new std::thread([this] {
            // make sure we set in any case, even exceptions
            auto guard = qScopeGuard([&] {
                this->stoppedFlag_.set();
            });

            runThread();
        }));

        renameThread(*this->mainThread_.get(), "BPSM-" % this->shortName_);
    }

    void stop()
    {
        if (this->stopping_)
        {
            return;
        }

        this->stopping_ = true;

        for (const auto &client : this->clients_)
        {
            client.second->close("Shutting down");
        }

        this->work_.reset();

        if (!this->mainThread_->joinable())
        {
            return;
        }

        // NOTE:
        // There is a case where a new client was initiated but not added to the clients list.
        // We just don't join the thread & let the operating system nuke the thread if joining fails
        // within 1s.
        if (this->stoppedFlag_.waitFor(std::chrono::milliseconds{100}))
        {
            this->mainThread_->join();
            return;
        }

        qCWarning(chatterinoLiveupdates)
            << "Thread didn't finish within 100ms, force-stop the client";
        this->websocketClient_.stop();
        if (this->stoppedFlag_.waitFor(std::chrono::milliseconds{20}))
        {
            this->mainThread_->join();
            return;
        }

        qCWarning(chatterinoLiveupdates)
            << "Thread didn't finish after stopping, discard it";
        // detach the thread so the destructor doesn't attempt any joining
        this->mainThread_->detach();
    }

protected:
    using WebsocketMessagePtr =
        websocketpp::config::asio_tls_client::message_type::ptr;
    using WebsocketContextPtr =
        websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

    virtual void onMessage(websocketpp::connection_hdl hdl,
                           WebsocketMessagePtr msg) = 0;

    virtual std::shared_ptr<BasicPubSubClient<Subscription>> createClient(
        liveupdates::WebsocketClient &client, websocketpp::connection_hdl hdl)
    {
        return std::make_shared<BasicPubSubClient<Subscription>>(client, hdl);
    }

    /**
     * @param hdl The handle of the client.
     * @return The client managing this connection, empty shared_ptr otherwise.
     */
    std::shared_ptr<BasicPubSubClient<Subscription>> findClient(
        websocketpp::connection_hdl hdl)
    {
        auto clientIt = this->clients_.find(hdl);

        if (clientIt == this->clients_.end())
        {
            return {};
        }

        return clientIt->second;
    }

    void unsubscribe(const Subscription &subscription)
    {
        for (auto &client : this->clients_)
        {
            if (client.second->unsubscribe(subscription))
            {
                return;
            }
        }
    }

    void subscribe(const Subscription &subscription)
    {
        if (this->trySubscribe(subscription))
        {
            return;
        }

        this->addClient();
        this->pendingSubscriptions_.emplace_back(subscription);
        DebugCount::increase("LiveUpdates subscription backlog");
    }

private:
    void onConnectionOpen(websocketpp::connection_hdl hdl)
    {
        DebugCount::increase("LiveUpdates connections");
        this->addingClient_ = false;
        this->diag.connectionsOpened.fetch_add(1, std::memory_order_acq_rel);

        this->connectBackoff_.reset();

        auto client = this->createClient(this->websocketClient_, hdl);

        // We separate the starting from the constructor because we will want to use
        // shared_from_this
        client->start();

        this->clients_.emplace(hdl, client);

        auto pendingSubsToTake = std::min(this->pendingSubscriptions_.size(),
                                          client->maxSubscriptions);

        qCDebug(chatterinoLiveupdates)
            << "LiveUpdate connection opened, subscribing to"
            << pendingSubsToTake << "subscriptions!";

        while (pendingSubsToTake > 0 && !this->pendingSubscriptions_.empty())
        {
            const auto last = std::move(this->pendingSubscriptions_.back());
            this->pendingSubscriptions_.pop_back();
            if (!client->subscribe(last))
            {
                qCDebug(chatterinoLiveupdates)
                    << "Failed to subscribe to" << last << "on new client.";
                // TODO: should we try to add a new client here?
                return;
            }
            DebugCount::decrease("LiveUpdates subscription backlog");
            pendingSubsToTake--;
        }

        if (!this->pendingSubscriptions_.empty())
        {
            this->addClient();
        }
    }

    void onConnectionFail(websocketpp::connection_hdl hdl)
    {
        DebugCount::increase("LiveUpdates failed connections");
        this->diag.connectionsFailed.fetch_add(1, std::memory_order_acq_rel);

        if (auto conn = this->websocketClient_.get_con_from_hdl(std::move(hdl)))
        {
            qCDebug(chatterinoLiveupdates)
                << "LiveUpdates connection attempt failed (error: "
                << conn->get_ec().message().c_str() << ")";
        }
        else
        {
            qCDebug(chatterinoLiveupdates)
                << "LiveUpdates connection attempt failed but we can't get the "
                   "connection from a handle.";
        }
        this->addingClient_ = false;
        if (!this->pendingSubscriptions_.empty())
        {
            runAfter(this->websocketClient_.get_io_service(),
                     this->connectBackoff_.next(), [this](auto /*timer*/) {
                         this->addClient();
                     });
        }
    }

    void onConnectionClose(websocketpp::connection_hdl hdl)
    {
        qCDebug(chatterinoLiveupdates) << "Connection closed";
        DebugCount::decrease("LiveUpdates connections");
        this->diag.connectionsClosed.fetch_add(1, std::memory_order_acq_rel);

        auto clientIt = this->clients_.find(hdl);

        // If this assert goes off, there's something wrong with the connection
        // creation/preserving code KKona
        assert(clientIt != this->clients_.end());

        auto client = clientIt->second;

        this->clients_.erase(clientIt);

        client->stop();

        if (!this->stopping_)
        {
            for (const auto &sub : client->subscriptions_)
            {
                this->subscribe(sub);
            }
        }
    }

    WebsocketContextPtr onTLSInit(const websocketpp::connection_hdl & /*hdl*/)
    {
        WebsocketContextPtr ctx(
            new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));

        try
        {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::single_dh_use);
        }
        catch (const std::exception &e)
        {
            qCDebug(chatterinoLiveupdates)
                << "Exception caught in OnTLSInit:" << e.what();
        }

        return ctx;
    }

    void runThread()
    {
        qCDebug(chatterinoLiveupdates) << "Start LiveUpdates manager thread";
        this->websocketClient_.run();
        qCDebug(chatterinoLiveupdates)
            << "Done with LiveUpdates manager thread";
    }

    void addClient()
    {
        if (this->addingClient_)
        {
            return;
        }

        qCDebug(chatterinoLiveupdates) << "Adding an additional client";

        this->addingClient_ = true;

        websocketpp::lib::error_code ec;
        auto con = this->websocketClient_.get_connection(
            this->host_.toStdString(), ec);

        if (ec)
        {
            qCDebug(chatterinoLiveupdates)
                << "Unable to establish connection:" << ec.message().c_str();
            return;
        }

        NetworkConfigurationProvider::applyToWebSocket(con);

        this->websocketClient_.connect(con);
    }

    bool trySubscribe(const Subscription &subscription)
    {
        for (auto &client : this->clients_)
        {
            if (client.second->subscribe(subscription))
            {
                return true;
            }
        }
        return false;
    }

    std::map<liveupdates::WebsocketHandle,
             std::shared_ptr<BasicPubSubClient<Subscription>>,
             std::owner_less<liveupdates::WebsocketHandle>>
        clients_;

    std::vector<Subscription> pendingSubscriptions_;
    std::atomic<bool> addingClient_{false};
    ExponentialBackoff<5> connectBackoff_{std::chrono::milliseconds(1000)};

    std::shared_ptr<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work_{nullptr};

    liveupdates::WebsocketClient websocketClient_;
    std::unique_ptr<std::thread> mainThread_;
    OnceFlag stoppedFlag_;

    const QString host_;

    /// Short name of the service (e.g. "7TV" or "BTTV")
    const QString shortName_;

    bool stopping_{false};
};

}  // namespace chatterino
