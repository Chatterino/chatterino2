#pragma once

#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubClient.hpp"
#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/twitch/PubSubMessages.hpp"
#include "providers/twitch/PubSubWebsocket.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/ExponentialBackoff.hpp"

#include <QJsonObject>
#include <QString>
#include <pajlada/signals/signal.hpp>
#include <websocketpp/client.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace chatterino {

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
    std::unique_ptr<std::thread> mainThread;

    // Account credentials
    // Set from setAccount or setAccountData
    QString token_;
    QString userID_;

public:
    // The max amount of connections we may open
    static constexpr int maxConnections = 10;

    PubSub(const QString &host,
           std::chrono::seconds pingInterval = std::chrono::seconds(15));

    void setAccount(std::shared_ptr<TwitchAccount> account)
    {
        this->token_ = account->getOAuthToken();
        this->userID_ = account->getUserId();
    }

    void setAccountData(QString token, QString userID)
    {
        this->token_ = token;
        this->userID_ = userID;
    }

    ~PubSub() = delete;

    enum class State {
        Connected,
        Disconnected,
    };

    void start();
    void stop();

    bool isConnected() const
    {
        return this->state == State::Connected;
    }

    struct {
        struct {
            Signal<ClearChatAction> chatCleared;
            Signal<DeleteAction> messageDeleted;
            Signal<ModeChangedAction> modeChanged;
            Signal<ModerationStateAction> moderationStateChanged;

            Signal<BanAction> userBanned;
            Signal<UnbanAction> userUnbanned;

            // Message caught by automod
            //                                channelID
            pajlada::Signals::Signal<PubSubAutoModQueueMessage, QString>
                autoModMessageCaught;

            // Message blocked by moderator
            Signal<AutomodAction> autoModMessageBlocked;

            Signal<AutomodUserAction> automodUserMessage;
            Signal<AutomodInfoAction> automodInfoMessage;
        } moderation;

        struct {
            // Parsing should be done in PubSubManager as well,
            // but for now we just send the raw data
            Signal<const PubSubWhisperMessage &> received;
            Signal<const PubSubWhisperMessage &> sent;
        } whisper;

        struct {
            Signal<const QJsonObject &> redeemed;
        } pointReward;
    } signals_;

    void unlistenAllModerationActions();
    void unlistenAutomod();
    void unlistenWhispers();

    bool listenToWhispers();
    void listenToChannelModerationActions(const QString &channelID);
    void listenToAutomod(const QString &channelID);

    void listenToChannelPointRewards(const QString &channelID);

    std::vector<QString> requests;

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

    void listenToTopic(const QString &topic);

private:
    void listen(PubSubListenMessage msg);
    bool tryListen(PubSubListenMessage msg);

    bool isListeningToTopic(const QString &topic);

    void addClient();
    std::atomic<bool> addingClient{false};
    ExponentialBackoff<5> connectBackoff{std::chrono::milliseconds(1000)};

    State state = State::Connected;

    std::map<WebsocketHandle, std::shared_ptr<PubSubClient>,
             std::owner_less<WebsocketHandle>>
        clients;

    std::unordered_map<
        QString, std::function<void(const QJsonObject &, const QString &)>>
        moderationActionHandlers;

    std::unordered_map<
        QString, std::function<void(const QJsonObject &, const QString &)>>
        channelTermsActionHandlers;

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
    boost::optional<NonceInfo> findNonceInfo(QString nonce);

    std::unordered_map<QString, NonceInfo> nonces_;

    void runThread();

    std::shared_ptr<boost::asio::io_service::work> work{nullptr};

    const QString host_;
    const PubSubClientOptions clientOptions_;

    bool stopping_{false};
};

}  // namespace chatterino
