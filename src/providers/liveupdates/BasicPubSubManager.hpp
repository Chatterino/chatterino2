#pragma once

#include "common/QLogging.hpp"
#include "common/websockets/WebSocketPool.hpp"
#include "providers/liveupdates/BasicPubSubListener.hpp"
#include "providers/liveupdates/Diag.hpp"
#include "util/DebugCount.hpp"
#include "util/ExponentialBackoff.hpp"

#include <QPointer>
#include <QTimer>

#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace chatterino {

namespace liveupdates {

template <typename Manager, typename Client>
concept IsManager = requires(Manager &manager) {
    { manager.makeClient() } -> std::same_as<std::shared_ptr<Client>>;
};

template <typename Client>
concept IsClient = requires(Client &client, const QByteArray &msg) {
    { client.onOpen() } -> std::same_as<void>;
    { client.onMessage(msg) } -> std::same_as<void>;
    { client.close() } -> std::same_as<void>;
};

}  // namespace liveupdates

/**
 * This class is the basis for connecting and interacting with
 * simple PubSub servers over the Websocket protocol.
 * It acts as a pool for connections (see BasicPubSubClient).
 *
 * You **must** implement a method `makeClient()` that returns a shared pointer
 * of the client. 
 *
 * You must expose your own subscribe and unsubscribe methods
 * (e.g. [un-]subscribeTopic).
 * This manager does not keep track of the subscriptions.
 *
 * @tparam Derived
 * The derived class. Used to dispatch to makeClient().
 *
 * @tparam ClientT
 * The client type. Must confirm to the IsClient above. Use BasicPubSubClient 
 * for a common implementation. Used to dispatch to the correct methods there 
 * and to get the subscription type.
 *
 * @see BasicPubSubClient
 */
template <typename Derived, typename ClientT>
class BasicPubSubManager : public QObject
{
public:
    using Subscription = ClientT::Subscription;
    using Client = ClientT;

    BasicPubSubManager(QString host, QString shortName)
        : pool_(std::make_optional<WebSocketPool>(shortName))
        , host_(std::move(host))
    {
        // We do this here, because `Derived` needs to be a complete type. If we
        // did it as a requires clause on the class, the type would be
        // incomplete.
        static_assert(liveupdates::IsManager<Derived, Client>);
        static_assert(liveupdates::IsClient<Client>);
    }

    ~BasicPubSubManager() override
    {
        // The derived class must call stop in its destructor
        assert(this->stopping_);
    }

    BasicPubSubManager(const BasicPubSubManager &) = delete;
    BasicPubSubManager(const BasicPubSubManager &&) = delete;
    BasicPubSubManager &operator=(const BasicPubSubManager &) = delete;
    BasicPubSubManager &operator=(const BasicPubSubManager &&) = delete;

    /** This is only used for testing. */
    liveupdates::Diag diag;

    void stop()
    {
        if (this->stopping_)
        {
            return;
        }

        this->stopping_ = true;
        this->pool_.reset();
    }

protected:
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

    const std::unordered_map<size_t, std::shared_ptr<Client>> &clients() const
    {
        return this->clients_;
    }

private:
    Derived *derived()
    {
        return static_cast<Derived *>(this);
    }

    void onConnectionOpen(size_t id)
    {
        DebugCount::increase("LiveUpdates connections");
        this->addingClient_ = false;
        this->diag.connectionsOpened.fetch_add(1, std::memory_order_acq_rel);

        this->connectBackoff_.reset();

        auto *client = this->resolve(id);
        client->onOpen();
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

    void onConnectionClose(size_t id)
    {
        this->addingClient_ = false;

        auto it = this->clients_.find(id);
        if (it == this->clients_.end())
        {
            qCWarning(chatterinoLiveupdates) << "Unknown client:" << id;
            assert(false);
            return;
        }

        DebugCount::decrease("LiveUpdates connections");
        qCDebug(chatterinoLiveupdates) << "Connection" << id << "closed";

        auto subs = std::move(it->second->subscriptions_);
        bool wasOpen = it->second->isOpen();

        if (wasOpen)
        {
            this->diag.connectionsClosed.fetch_add(1,
                                                   std::memory_order::relaxed);
        }
        else
        {
            this->diag.connectionsFailed.fetch_add(1,
                                                   std::memory_order::relaxed);
        }

        this->clients_.erase(it);
        if (this->stopping_)
        {
            return;
        }

        if (!wasOpen)
        {
            qCWarning(chatterinoLiveupdates)
                << "Retrying after" << id << "failed";
            this->pendingSubscriptions_.insert(
                this->pendingSubscriptions_.end(),
                std::make_move_iterator(subs.begin()),
                std::make_move_iterator(subs.end()));
            QTimer::singleShot(this->connectBackoff_.next(), this, [this] {
                this->addClient();
            });
            return;
        }

        for (const auto &sub : subs)
        {
            this->subscribe(sub);
        }
    }

    void addClient()
    {
        if (this->addingClient_ || !this->pool_)
        {
            return;
        }

        qCDebug(chatterinoLiveupdates) << "Adding an additional client";

        this->addingClient_ = true;

        auto id = this->nextId_++;
        auto client = this->derived()->makeClient();
        auto hdl = this->pool_->createSocket(
            WebSocketOptions{
                .url = this->host_,
                .headers = {},
            },
            std::make_unique<BasicPubSubListener<Derived>>(
                std::weak_ptr{client}, this->derived(), id));
        client->ws_ = std::move(hdl);
        this->clients_.emplace(id, std::move(client));
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

    Client *resolve(size_t id)
    {
        auto it = this->clients_.find(id);
        if (it == this->clients_.end())
        {
            return nullptr;
        }
        return it->second.get();
    }

    std::vector<Subscription> pendingSubscriptions_;
    ExponentialBackoff<5> connectBackoff_{std::chrono::milliseconds(1000)};

    std::optional<WebSocketPool> pool_;
    std::unordered_map<size_t, std::shared_ptr<Client>> clients_;

    const QString host_;

    size_t nextId_ = 0;

    bool stopping_ = false;
    bool addingClient_ = false;

    friend BasicPubSubListener<Derived>;
};

}  // namespace chatterino
