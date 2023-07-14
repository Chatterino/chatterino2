#pragma once

#include "common/QLogging.hpp"
#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/ws/Client.hpp"
#include "util/DebugCount.hpp"
#include "util/ExponentialBackoff.hpp"

#include <pajlada/signals/signal.hpp>
#include <QJsonObject>
#include <QString>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <unordered_map>
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
class BasicPubSubManager : public ws::Client
{
public:
    BasicPubSubManager(QString host)
        : host_(std::move(host))
    {
    }

    ~BasicPubSubManager() override
    {
        this->stop();
    };

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

    void stop()
    {
        this->stopping_ = true;

        for (const auto &client : this->clients_)
        {
            client.second->close("Shutting down");
        }

        Client::stop();

        assert(this->clients_.empty());
    }

protected:
    virtual std::shared_ptr<BasicPubSubClient<Subscription>> createClient(
        ws::Client *client, const ws::Connection &conn)
    {
        return std::make_shared<BasicPubSubClient<Subscription>>(client, conn);
    }

    /**
     * @param hdl The handle of the client.
     * @return The client managing this connection, empty shared_ptr otherwise.
     */
    std::shared_ptr<BasicPubSubClient<Subscription>> findClient(
        const ws::Connection &conn)
    {
        auto clientIt = this->clients_.find(conn);

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
    void onConnectionOpen(const ws::Connection &conn) override
    {
        DebugCount::increase("LiveUpdates connections");
        this->addingClient_ = false;
        this->diag.connectionsOpened.fetch_add(1, std::memory_order_acq_rel);

        this->connectBackoff_.reset();

        auto client = this->createClient(this, conn);

        // We separate the starting from the constructor because we will want to use
        // shared_from_this
        client->start();

        this->clients_.emplace(conn, client);

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

    void onConnectionFailed(QLatin1String reason) override
    {
        DebugCount::increase("LiveUpdates failed connections");
        this->diag.connectionsFailed.fetch_add(1, std::memory_order_acq_rel);

        qCDebug(chatterinoLiveupdates)
            << "LiveUpdates connection attempt failed (error: " << reason
            << ")";

        this->addingClient_ = false;
        if (!this->pendingSubscriptions_.empty())
        {
            this->runAfter(this->connectBackoff_.next(), [this]() {
                this->addClient();
            });
        }
    }

    void onConnectionClosed(const ws::Connection &conn) override
    {
        qCDebug(chatterinoLiveupdates) << "Connection closed";
        DebugCount::decrease("LiveUpdates connections");
        this->diag.connectionsClosed.fetch_add(1, std::memory_order_acq_rel);

        auto clientIt = this->clients_.find(conn);

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

    void addClient()
    {
        if (this->addingClient_)
        {
            return;
        }

        qCDebug(chatterinoLiveupdates) << "Adding an additional client";

        this->addingClient_ = true;

        this->addConnection(this->host_);
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

    std::map<ws::Connection, std::shared_ptr<BasicPubSubClient<Subscription>>>
        clients_;

    std::vector<Subscription> pendingSubscriptions_;
    std::atomic<bool> addingClient_{false};
    ExponentialBackoff<5> connectBackoff_{std::chrono::milliseconds(1000)};

    const QString host_;

    bool stopping_{false};
};

}  // namespace chatterino
