#pragma once

#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/PubsubClientOptions.hpp"
#include "providers/twitch/PubsubMessages.hpp"
#include "providers/twitch/PubsubWebsocket.hpp"

#include <QString>
#include <pajlada/signals/signal.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

#include <atomic>
#include <vector>

namespace chatterino {

struct TopicData {
    QString topic;
    bool authed{false};
    bool persistent{false};
};

struct Listener : TopicData {
    bool confirmed{false};
};

class PubSubClient : public std::enable_shared_from_this<PubSubClient>
{
public:
    // The max amount of topics we may listen to with a single connection
    static constexpr std::size_t listensPerConnection = 50;

    PubSubClient(WebsocketClient &_websocketClient, WebsocketHandle _handle,
                 const PubSubClientOptions &clientOptions);

    void start();
    void stop();

    void close(const std::string &reason,
               websocketpp::close::status::value code =
                   websocketpp::close::status::normal);

    bool listen(PubSubListenMessage msg);
    void unlistenPrefix(const QString &prefix);

    void handleListenResponse(const PubSubMessage &message);
    void handleUnlistenResponse(const PubSubMessage &message);

    void handlePong();

    bool isListeningToTopic(const QString &topic);

    std::vector<Listener> getListeners() const;

private:
    void ping();
    bool send(const char *payload);

    WebsocketClient &websocketClient_;
    WebsocketHandle handle_;
    uint16_t numListens_ = 0;

    std::vector<Listener> listeners_;

    std::atomic<bool> awaitingPong_{false};
    std::atomic<bool> started_{false};

    const PubSubClientOptions &clientOptions_;
};

}  // namespace chatterino
