#pragma once

#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/ws/Client.hpp"

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

    PubSubClient(ws::Client *client, ws::Connection conn,
                 const PubSubClientOptions &clientOptions);

    void start();
    void stop();

    void close(const QString &reason);

    bool listen(PubSubListenMessage msg);
    UnlistenPrefixResponse unlistenPrefix(const QString &prefix);

    void handleListenResponse(const PubSubMessage &message);
    void handleUnlistenResponse(const PubSubMessage &message);

    void handlePong();

    bool isListeningToTopic(const QString &topic);

    std::vector<Listener> getListeners() const;

private:
    void ping();

    ws::Client *client_;
    ws::Connection connection_;
    uint16_t numListens_ = 0;

    std::vector<Listener> listeners_;

    std::atomic<bool> awaitingPong_{false};
    std::atomic<bool> started_{false};

    const PubSubClientOptions &clientOptions_;
};

}  // namespace chatterino
