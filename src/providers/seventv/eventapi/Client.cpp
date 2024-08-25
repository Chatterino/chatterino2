#include "providers/seventv/eventapi/Client.hpp"

#include "providers/seventv/eventapi/Subscription.hpp"
#include "providers/twitch/PubSubHelpers.hpp"

#include <utility>

namespace chatterino::seventv::eventapi {

Client::Client(liveupdates::WebsocketClient &websocketClient,
               liveupdates::WebsocketHandle handle,
               std::chrono::milliseconds heartbeatInterval)
    : BasicPubSubClient<Subscription>(websocketClient, std::move(handle))
    , lastHeartbeat_(std::chrono::steady_clock::now())
    , heartbeatInterval_(heartbeatInterval)
    , heartbeatTimer_(std::make_shared<boost::asio::steady_timer>(
          this->websocketClient_.get_io_service()))
{
}

void Client::stopImpl()
{
    this->heartbeatTimer_->cancel();
}

void Client::onConnectionEstablished()
{
    this->lastHeartbeat_.store(std::chrono::steady_clock::now(),
                               std::memory_order_release);
    this->checkHeartbeat();
}

void Client::setHeartbeatInterval(int intervalMs)
{
    qCDebug(chatterinoSeventvEventAPI)
        << "Setting expected heartbeat interval to" << intervalMs << "ms";
    this->heartbeatInterval_ = std::chrono::milliseconds(intervalMs);
}

void Client::handleHeartbeat()
{
    this->lastHeartbeat_.store(std::chrono::steady_clock::now(),
                               std::memory_order_release);
}

void Client::checkHeartbeat()
{
    // Following the heartbeat docs, a connection is dead
    // after three missed heartbeats.
    // https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#heartbeat
    assert(this->isStarted());
    if ((std::chrono::steady_clock::now() - this->lastHeartbeat_.load()) >
        3 * this->heartbeatInterval_)
    {
        qCDebug(chatterinoSeventvEventAPI)
            << "Didn't receive a heartbeat in time, disconnecting!";
        this->close("Didn't receive a heartbeat in time");

        return;
    }

    auto self = std::dynamic_pointer_cast<Client>(this->shared_from_this());

    runAfter(this->heartbeatTimer_, this->heartbeatInterval_, [self](auto) {
        if (!self->isStarted())
        {
            return;
        }
        self->checkHeartbeat();
    });
}

}  // namespace chatterino::seventv::eventapi
