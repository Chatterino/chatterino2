#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/seventv/eventapi/SeventvEventAPISubscription.hpp"

namespace chatterino {

class SeventvEventAPIClient
    : public BasicPubSubClient<SeventvEventAPISubscription>
{
public:
    SeventvEventAPIClient(liveupdates::WebsocketClient &websocketClient,
                          liveupdates::WebsocketHandle handle,
                          std::chrono::milliseconds heartbeatInterval);

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

    friend class SeventvEventAPI;
};

}  // namespace chatterino
