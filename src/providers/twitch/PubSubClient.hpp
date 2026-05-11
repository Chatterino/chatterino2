// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/twitch/pubsubmessages/Message.hpp"

#include <pajlada/signals/signal.hpp>
#include <QString>

#include <atomic>
#include <vector>

namespace chatterino {

struct TopicData {
    QString topic;

    bool operator==(const TopicData &other) const
    {
        return this->topic == other.topic;
    }

    friend QDebug operator<<(QDebug debug, const TopicData &data);
};

}  // namespace chatterino

template <>
struct std::hash<chatterino::TopicData> {
    std::size_t operator()(const chatterino::TopicData &data) const noexcept
    {
        return std::hash<QString>{}(data.topic);
    }
};

namespace chatterino {

struct PubSubMessage;
class PubSub;
struct PubSubListenMessage;

class PubSubClient : public BasicPubSubClient<TopicData, PubSubClient>
{
public:
    // The max amount of topics we may listen to with a single connection
    static constexpr size_t MAX_LISTENS = 50;

    struct UnlistenPrefixResponse {
        std::vector<QString> topics;
        QString nonce;
    };

    PubSubClient(PubSub &manager, std::chrono::milliseconds heartbeatInterval);

    void onOpen() /* override */;
    void onMessage(const QByteArray &msg) /* override */;

    void checkHeartbeat();

    QByteArray encodeSubscription(
        const Subscription &subscription) /* override */;
    QByteArray encodeUnsubscription(
        const Subscription &subscription) /* override */;

private:
    struct NonceInfo {
        bool isListen;
    };

    void handleResponse(const PubSubMessage &message);
    void handleMessageResponse(const PubSubMessageMessage &message);

    std::unordered_map<QString, NonceInfo> nonces_;

    std::atomic<std::chrono::time_point<std::chrono::steady_clock>>
        lastHeartbeat_;
    std::chrono::milliseconds heartbeatInterval_;
    bool isOpen_ = false;
    PubSub &manager_;
};

}  // namespace chatterino
