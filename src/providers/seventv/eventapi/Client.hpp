#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
// this needs to be included for the specialization
// of std::hash for Subscription
#include "providers/seventv/eventapi/Subscription.hpp"

namespace chatterino {
class SeventvEventAPI;

}  // namespace chatterino

namespace chatterino::seventv::eventapi {

class Client : public BasicPubSubClient<Subscription>
{
public:
    Client(liveupdates::WebsocketClient &websocketClient,
           liveupdates::WebsocketHandle handle,
           std::chrono::milliseconds heartbeatInterval);

    void stopImpl() override;

    void setHeartbeatInterval(int intervalMs);
    void handleHeartbeat();

protected:
    void onConnectionEstablished() override;

private:
    void checkHeartbeat();

    std::atomic<std::chrono::time_point<std::chrono::steady_clock>>
        lastHeartbeat_;
    // This will be set once on the welcome message.
    std::chrono::milliseconds heartbeatInterval_;
    std::shared_ptr<boost::asio::steady_timer> heartbeatTimer_;

    friend SeventvEventAPI;
};

}  // namespace chatterino::seventv::eventapi
