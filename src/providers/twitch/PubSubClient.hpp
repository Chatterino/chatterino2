#pragma once

#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/twitch/PubSubWebsocket.hpp"

#include <pajlada/signals/signal.hpp>
#include <QString>

#include <atomic>
#include <vector>

namespace chatterino {

struct PubSubMessage;
struct PubSubListenMessage;

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
    struct UnlistenPrefixResponse {
        std::vector<QString> topics;
        QString nonce;
    };

    // The max amount of topics we may listen to with a single connection
    static constexpr std::vector<QString>::size_type MAX_LISTENS = 50;

    PubSubClient(WebsocketClient &_websocketClient, WebsocketHandle _handle,
                 const PubSubClientOptions &clientOptions);

    void start();
    void stop();

    void close(const std::string &reason,
               websocketpp::close::status::value code =
                   websocketpp::close::status::normal);

    bool listen(const PubSubListenMessage &msg);
    UnlistenPrefixResponse unlistenPrefix(const QString &prefix);

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

    std::shared_ptr<boost::asio::steady_timer> heartbeatTimer_;
    const PubSubClientOptions &clientOptions_;
};

}  // namespace chatterino
