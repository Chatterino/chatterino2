#pragma once

#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/twitch/PubSubWebsocket.hpp"
#include "util/ExponentialBackoff.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>
#include <QJsonObject>
#include <QString>
#include <websocketpp/client.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace chatterino {

class TwitchAccount;
class PubSubClient;

struct ClearChatAction;
struct DeleteAction;
struct ModeChangedAction;
struct ModerationStateAction;
struct BanAction;
struct UnbanAction;
struct PubSubAutoModQueueMessage;
struct AutomodAction;
struct AutomodUserAction;
struct AutomodInfoAction;
struct PubSubLowTrustUsersMessage;
struct PubSubWhisperMessage;

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

    // Account credentials
    // Set from setAccount
    QString token_;
    QString userID_;

public:
    PubSub(const QString &host,
           std::chrono::seconds pingInterval = std::chrono::seconds(15));
    ~PubSub();

    PubSub(const PubSub &) = delete;
    PubSub(PubSub &&) = delete;
    PubSub &operator=(const PubSub &) = delete;
    PubSub &operator=(PubSub &&) = delete;

    void setAccount(std::shared_ptr<TwitchAccount> account);

    void start();
    void stop();

    struct {
        Signal<ClearChatAction> chatCleared;
        Signal<DeleteAction> messageDeleted;
        Signal<ModeChangedAction> modeChanged;
        Signal<ModerationStateAction> moderationStateChanged;

        Signal<BanAction> userBanned;
        Signal<UnbanAction> userUnbanned;

        Signal<PubSubLowTrustUsersMessage> suspiciousMessageReceived;
        Signal<PubSubLowTrustUsersMessage> suspiciousTreatmentUpdated;

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

    /**
     * Listen to incoming whispers for the currently logged in user.
     * This topic is relevant for everyone.
     *
     * PubSub topic: whispers.{currentUserID}
     */
    bool listenToWhispers();
    void unlistenWhispers();

    /**
     * Listen to moderation actions in the given channel.
     * This topic is relevant for everyone.
     * For moderators, this topic includes blocked/permitted terms updates,
     * roomstate changes, general mod/vip updates, all bans/timeouts/deletions.
     * For normal users, this topic includes moderation actions that are targetted at the local user:
     * automod catching a user's sent message, a moderator approving or denying their caught messages,
     * the user gaining/losing mod/vip, the user receiving a ban/timeout/deletion.
     *
     * PubSub topic: chat_moderator_actions.{currentUserID}.{channelID}
     */
    void listenToChannelModerationActions(const QString &channelID);
    void unlistenChannelModerationActions();

    /**
     * Listen to Automod events in the given channel.
     * This topic is only relevant for moderators.
     * This will send events about incoming messages that
     * are caught by Automod.
     *
     * PubSub topic: automod-queue.{currentUserID}.{channelID}
     */
    void listenToAutomod(const QString &channelID);
    void unlistenAutomod();

    /**
     * Listen to Low Trust events in the given channel.
     * This topic is only relevant for moderators.
     * This will fire events about suspicious treatment updates
     * and messages sent by restricted/monitored users.
     *
     * PubSub topic: low-trust-users.{currentUserID}.{channelID}
     */
    void listenToLowTrustUsers(const QString &channelID);
    void unlistenLowTrustUsers();

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
    std::optional<NonceInfo> findNonceInfo(QString nonce);

    std::unordered_map<QString, NonceInfo> nonces_;

    void runThread();

    std::shared_ptr<boost::asio::io_service::work> work{nullptr};

    const QString host_;
    const PubSubClientOptions clientOptions_;

    bool stopping_{false};
};

}  // namespace chatterino
