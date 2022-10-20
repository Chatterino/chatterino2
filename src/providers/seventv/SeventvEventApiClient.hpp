#pragma once

#include <QString>
#include <pajlada/signals/signal.hpp>
#include "providers/seventv/SeventvEventApi.hpp"
#include "providers/seventv/SeventvEventApiWebsocket.hpp"

#include <atomic>
#include <unordered_set>

namespace chatterino {
class SeventvEventApiClient
    : public std::enable_shared_from_this<SeventvEventApiClient>
{
public:
    // The max amount of channels we may join with a single connection
    static constexpr std::vector<QString>::size_type MAX_LISTENS = 100;

    SeventvEventApiClient(eventapi::WebsocketClient &_websocketClient,
                          eventapi::WebsocketHandle _handle);

    void start();
    void stop();

    void close(const std::string &reason,
               websocketpp::close::status::value code =
                   websocketpp::close::status::normal);

    bool subscribe(const SeventvEventApiSubscription &subscription);
    bool unsubscribe(const SeventvEventApiSubscription &subscription);

    void setHeartbeatInterval(int intervalMs);
    void handleHeartbeat();

    bool isSubscribedToEmoteSet(const QString &emoteSetId);

    std::unordered_set<SeventvEventApiSubscription> getSubscriptions() const;

private:
    void checkHeartbeat();
    bool send(const char *payload);

    eventapi::WebsocketClient &websocketClient_;
    eventapi::WebsocketHandle handle_;
    std::unordered_set<SeventvEventApiSubscription> subscriptions_;

    std::atomic<std::chrono::time_point<std::chrono::steady_clock>>
        lastHeartbeat_;
    std::atomic<std::chrono::milliseconds> heartbeatInterval_;
    std::atomic<bool> started_{false};
};
}  // namespace chatterino
