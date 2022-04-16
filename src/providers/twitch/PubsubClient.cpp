#include "providers/twitch/PubsubClient.hpp"

#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/PubsubHelpers.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/Helpers.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/error/en.h>

#include <exception>
#include <iostream>
#include <thread>
#include "common/QLogging.hpp"

namespace chatterino {

static const char *PING_PAYLOAD = "{\"type\":\"PING\"}";

PubSubClient::PubSubClient(WebsocketClient &websocketClient,
                           WebsocketHandle handle,
                           const PubSubClientOptions &clientOptions)
    : websocketClient_(websocketClient)
    , handle_(handle)
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

void PubSubClient::close(const std::string &reason,
                         websocketpp::close::status::value code)
{
    WebsocketErrorCode ec;

    auto conn = this->websocketClient_.get_con_from_hdl(this->handle_, ec);
    if (ec)
    {
        qCDebug(chatterinoPubsub)
            << "Client::close(): Error getting con:" << ec.message().c_str();
        return;
    }

    conn->close(code, reason, ec);
    if (ec)
    {
        qCDebug(chatterinoPubsub)
            << "Client::close(): Error closing:" << ec.message().c_str();
        return;
    }
}

std::pair<bool, QString> PubSubClient::listen(rapidjson::Document &message)
{
    int numRequestedListens = message["data"]["topics"].Size();

    if (this->numListens_ + numRequestedListens >
        PubSubClient::listensPerConnection)
    {
        // This PubSubClient is already at its peak listens
        return {false, ""};
    }
    this->numListens_ += numRequestedListens;
    DebugCount::increase("PubSub topic pending listens", numRequestedListens);

    for (const auto &topic : message["data"]["topics"].GetArray())
    {
        this->listeners_.emplace_back(
            Listener{topic.GetString(), false, false, false});
    }

    qCDebug(chatterinoPubsub)
        << "Subscribing to" << numRequestedListens << "topics";

    auto nonce = generateUuid();
    rj::set(message, "nonce", nonce);

    QString payload = rj::stringify(message);
    // sentListens[nonce] = RequestMessage{payload, numRequestedListens};

    this->send(payload.toUtf8());

    return {true, nonce};
}

void PubSubClient::unlistenPrefix(const QString &prefix)
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
        return;
    }

    int numRequestedUnlistens = topics.size();

    this->numListens_ -= numRequestedUnlistens;
    DebugCount::increase("PubSub topic pending unlistens",
                         numRequestedUnlistens);

    auto message = createUnlistenMessage(topics);

    auto nonce = generateUuid();
    rj::set(message, "nonce", nonce);

    QString payload = rj::stringify(message);
    // TODO
    // sentUnlistens[nonce] = RequestMessage{payload, numRequestedUnlistens};

    this->send(payload.toUtf8());
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
        qCDebug(chatterinoPubsub) << "No pong response, disconnect!";
        this->close("Didn't respond to ping");

        return;
    }

    if (!this->send(PING_PAYLOAD))
    {
        return;
    }

    this->awaitingPong_ = true;

    auto self = this->shared_from_this();

    runAfter(this->websocketClient_.get_io_service(),
             this->clientOptions_.pingInterval_, [self](auto timer) {
                 if (!self->started_)
                 {
                     return;
                 }

                 self->ping();
             });
}

bool PubSubClient::send(const char *payload)
{
    WebsocketErrorCode ec;
    this->websocketClient_.send(this->handle_, payload,
                                websocketpp::frame::opcode::text, ec);

    if (ec)
    {
        qCDebug(chatterinoPubsub) << "Error sending message" << payload << ":"
                                  << ec.message().c_str();
        // TODO(pajlada): Check which error code happened and maybe
        // gracefully handle it

        return false;
    }

    return true;
}

}  // namespace chatterino
