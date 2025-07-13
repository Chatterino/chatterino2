#pragma once

#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/twitch/PubSubWebsocket.hpp"
#include "util/ExponentialBackoff.hpp"
#include "util/OnceFlag.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <pajlada/signals/signal.hpp>
#include <QJsonObject>
#include <QString>
#include <websocketpp/client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#if __has_include(<gtest/gtest_prod.h>)
#    include <gtest/gtest_prod.h>
#endif

namespace chatterino {

class TwitchAccount;
class PubSubClient;

struct PubSubListenMessage;
struct PubSubMessage;
struct PubSubMessageMessage;

/**
 * This handles the Twitch PubSub connection
 *
 * Known issues:
 *  - Upon closing a channel, we don't unsubscribe to its pubsub connections
 *  - Stop is never called, meaning we never do a clean shutdown
 */
class PubSub
{
    using WebsocketMessagePtr =
        websocketpp::config::asio_tls_client::message_type::ptr;
    using WebsocketContextPtr =
        websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

    struct NonceInfo {
        std::weak_ptr<PubSubClient> client;
        QString messageType;  // e.g. LISTEN or UNLISTEN
        std::vector<QString> topics;
        std::vector<QString>::size_type topicCount;
    };

    WebsocketClient websocketClient;
    std::unique_ptr<std::thread> thread;

public:
    PubSub(const QString &host,
           std::chrono::seconds pingInterval = std::chrono::seconds(15));
    ~PubSub();

    PubSub(const PubSub &) = delete;
    PubSub(PubSub &&) = delete;
    PubSub &operator=(const PubSub &) = delete;
    PubSub &operator=(PubSub &&) = delete;

    /// Set up connections between itself & other parts of the application
    void initialize();

    struct {
        Signal<const QJsonObject &> redeemed;
    } pointReward;

    /**
     * Listen to incoming channel point redemptions in the given channel.
     * This topic is relevant for everyone.
     *
     * PubSub topic: community-points-channel-v1.{channelID}
     */
    void listenToChannelPointRewards(const QString &channelID);
    void unlistenChannelPointRewards();

    struct {
        std::atomic<uint32_t> connectionsClosed{0};
        std::atomic<uint32_t> connectionsOpened{0};
        std::atomic<uint32_t> connectionsFailed{0};
        std::atomic<uint32_t> messagesReceived{0};
        std::atomic<uint32_t> messagesFailedToParse{0};
        std::atomic<uint32_t> failedListenResponses{0};
        std::atomic<uint32_t> listenResponses{0};
        std::atomic<uint32_t> unlistenResponses{0};
    } diag;

private:
    void start();
    void stop();

    /**
     * Unlistens to all topics matching the prefix in all clients
     */
    void unlistenPrefix(const QString &prefix);

    void listenToTopic(const QString &topic);

    void listen(PubSubListenMessage msg);
    bool tryListen(PubSubListenMessage msg);

    bool isListeningToTopic(const QString &topic);

    void addClient();

    std::vector<QString> requests;

    std::atomic<bool> addingClient{false};
    ExponentialBackoff<5> connectBackoff{std::chrono::milliseconds(1000)};

    std::map<WebsocketHandle, std::shared_ptr<PubSubClient>,
             std::owner_less<WebsocketHandle>>
        clients;

    void onMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr msg);
    void onConnectionOpen(websocketpp::connection_hdl hdl);
    void onConnectionFail(websocketpp::connection_hdl hdl);
    void onConnectionClose(websocketpp::connection_hdl hdl);
    WebsocketContextPtr onTLSInit(websocketpp::connection_hdl hdl);

    void handleResponse(const PubSubMessage &message);
    void handleListenResponse(const NonceInfo &info, bool failed);
    void handleUnlistenResponse(const NonceInfo &info, bool failed);
    void handleMessageResponse(const PubSubMessageMessage &message);

    // Register a nonce for a specific client
    void registerNonce(QString nonce, NonceInfo nonceInfo);

    // Find client associated with a nonce
    std::optional<NonceInfo> findNonceInfo(QString nonce);

    std::unordered_map<QString, NonceInfo> nonces_;

    void runThread();

    std::shared_ptr<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work{nullptr};

    const QString host_;
    const PubSubClientOptions clientOptions_;

    OnceFlag stoppedFlag_;

    bool stopping_{false};

#ifdef FRIEND_TEST
    friend class FTest;

    FRIEND_TEST(TwitchPubSubClient, ServerRespondsToPings);
    FRIEND_TEST(TwitchPubSubClient, ServerDoesntRespondToPings);
    FRIEND_TEST(TwitchPubSubClient, DisconnectedAfter1s);
    FRIEND_TEST(TwitchPubSubClient, ExceedTopicLimit);
    FRIEND_TEST(TwitchPubSubClient, ExceedTopicLimitSingleStep);
#endif
};

}  // namespace chatterino
