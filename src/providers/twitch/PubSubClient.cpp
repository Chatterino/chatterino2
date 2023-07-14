#include "providers/twitch/PubSubClient.hpp"

#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/pubsubmessages/Listen.hpp"
#include "providers/twitch/pubsubmessages/Unlisten.hpp"
#include "util/DebugCount.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <exception>
#include <thread>

namespace chatterino {

using namespace literals;

constexpr const QLatin1String PING_PAYLOAD = R"({"type":"PING"})"_L1;

PubSubClient::PubSubClient(ws::Client *client, ws::Connection conn,
                           const PubSubClientOptions &clientOptions)
    : client_(client)
    , connection_(std::move(conn))
    , clientOptions_(clientOptions)
{
}

void PubSubClient::start()
{
    assert(!this->started_);

    this->started_ = true;

    this->ping();
}

void PubSubClient::stop()
{
    assert(this->started_);

    this->started_ = false;
}

void PubSubClient::close(const QString &reason)
{
    this->client_->close(this->connection_, reason);
}

bool PubSubClient::listen(PubSubListenMessage msg)
{
    int numRequestedListens = msg.topics.size();

    if (this->numListens_ + numRequestedListens > PubSubClient::MAX_LISTENS)
    {
        // This PubSubClient is already at its peak listens
        return false;
    }
    this->numListens_ += numRequestedListens;
    DebugCount::increase("PubSub topic pending listens", numRequestedListens);

    for (const auto &topic : msg.topics)
    {
        this->listeners_.emplace_back(Listener{topic, false, false, false});
    }

    qCDebug(chatterinoPubSub)
        << "Subscribing to" << numRequestedListens << "topics";

    this->client_->sendText(this->connection_, msg.toJson());

    return true;
}

PubSubClient::UnlistenPrefixResponse PubSubClient::unlistenPrefix(
    const QString &prefix)
{
    std::vector<QString> topics;

    for (auto it = this->listeners_.begin(); it != this->listeners_.end();)
    {
        const auto &listener = *it;
        if (listener.topic.startsWith(prefix))
        {
            topics.push_back(listener.topic);
            it = this->listeners_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (topics.empty())
    {
        return {{}, ""};
    }

    auto numRequestedUnlistens = topics.size();

    this->numListens_ -= numRequestedUnlistens;
    DebugCount::increase("PubSub topic pending unlistens",
                         numRequestedUnlistens);

    PubSubUnlistenMessage message(topics);

    this->client_->sendText(this->connection_, message.toJson());

    return {message.topics, message.nonce};
}

void PubSubClient::handleListenResponse(const PubSubMessage &message)
{
}

void PubSubClient::handleUnlistenResponse(const PubSubMessage &message)
{
}

void PubSubClient::handlePong()
{
    assert(this->awaitingPong_);

    this->awaitingPong_ = false;
}

bool PubSubClient::isListeningToTopic(const QString &topic)
{
    for (const auto &listener : this->listeners_)
    {
        if (listener.topic == topic)
        {
            return true;
        }
    }

    return false;
}

std::vector<Listener> PubSubClient::getListeners() const
{
    return this->listeners_;
}

void PubSubClient::ping()
{
    assert(this->started_);

    if (this->awaitingPong_)
    {
        qCDebug(chatterinoPubSub) << "No pong response, disconnect!";
        this->close("Didn't respond to ping");

        return;
    }

    if (!this->client_->sendText(this->connection_, PING_PAYLOAD))
    {
        return;
    }

    this->awaitingPong_ = true;

    auto self = this->shared_from_this();

    this->client_->runAfter(this->clientOptions_.pingInterval_, [self]() {
        if (!self->started_)
        {
            return;
        }

        self->ping();
    });
}

}  // namespace chatterino
