#pragma once

#include <QString>
#include <pajlada/signals/signal.hpp>
#include "providers/seventv/SeventvEventApiMessages.hpp"
#include "providers/seventv/SeventvEventApiWebsocket.hpp"

#include <atomic>
#include <vector>

namespace chatterino {
struct EventApiListener {
    QString channel;
};

class SeventvEventApiClient
    : public std::enable_shared_from_this<SeventvEventApiClient>
{
public:
    // The max amount of channels we may join with a single connection
    static constexpr std::vector<QString>::size_type MAX_LISTENS = 100;
    // The server has to send at least one ping during this interval.
    // After this time, the client checks if it received a ping.
    static constexpr std::chrono::seconds CHECK_PING_INTERVAL =
        std::chrono::seconds(60);

    SeventvEventApiClient(eventapi::WebsocketClient &_websocketClient,
                          eventapi::WebsocketHandle _handle);

    void start();
    void stop();

    void close(const std::string &reason,
               websocketpp::close::status::value code =
                   websocketpp::close::status::normal);

    bool join(const QString &channel);
    void part(const QString &channel);

    void handlePing();

    bool isJoinedChannel(const QString &channel);

    std::vector<EventApiListener> getListeners() const;

private:
    void checkPing();
    bool send(const char *payload);

    eventapi::WebsocketClient &websocketClient_;
    eventapi::WebsocketHandle handle_;
    std::vector<EventApiListener> channels;

    std::atomic<std::chrono::time_point<std::chrono::steady_clock>> lastPing_;
    std::atomic<bool> started_{false};
};
}  // namespace chatterino
