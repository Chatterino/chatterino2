#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/seventv/eventapi/SeventvEventApiSubscription.hpp"

namespace chatterino {

class SeventvEventApiClient
    : public BasicPubSubClient<SeventvEventApiSubscription>
{
public:
    SeventvEventApiClient(liveupdates::WebsocketClient &websocketClient,
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
    std::atomic<std::chrono::milliseconds> heartbeatInterval_;

    friend class SeventvEventApi;
};

}  // namespace chatterino
