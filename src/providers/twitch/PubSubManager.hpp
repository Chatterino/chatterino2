#pragma once

#include "providers/twitch/PubSubClientOptions.hpp"
#include "providers/ws/Client.hpp"
#include "util/ExponentialBackoff.hpp"
#include "util/QStringHash.hpp"

#include <boost/optional.hpp>
#include <pajlada/signals/signal.hpp>
#include <QJsonObject>
#include <QString>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
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
struct PubSubWhisperMessage;

struct PubSubListenMessage;
struct PubSubMessage;
struct PubSubMessageMessage;

class PubSub : public ws::Client
{
    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

    struct NonceInfo {
        std::weak_ptr<PubSubClient> client;
        QString messageType;  // e.g. LISTEN or UNLISTEN
        std::vector<QString> topics;
        std::vector<QString>::size_type topicCount;
    };

    // Account credentials
    // Set from setAccount or setAccountData
    QString token_;
    QString userID_;

public:
    // The max amount of connections we may open
    static constexpr int maxConnections = 10;

    PubSub(const QString &host,
           std::chrono::seconds pingInterval = std::chrono::seconds(15));

    void setAccount(std::shared_ptr<TwitchAccount> account);

    void setAccountData(QString token, QString userID);

    ~PubSub() override;

    enum class State {
        Connected,
        Disconnected,
    };

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

protected:
    void onConnectionClosed(const ws::Connection &conn) override;
    void onConnectionFailed(QLatin1String reason) override;
    void onConnectionOpen(const ws::Connection &conn) override;
    void onTextMessage(const ws::Connection &conn,
                       const QLatin1String &data) override;

private:
    void listen(PubSubListenMessage msg);
    bool tryListen(PubSubListenMessage msg);

    bool isListeningToTopic(const QString &topic);

    void addClient();
    std::atomic<bool> addingClient{false};
    ExponentialBackoff<5> connectBackoff{std::chrono::milliseconds(1000)};

    State state = State::Connected;

    std::map<ws::Connection, std::shared_ptr<PubSubClient>> clients;

    std::unordered_map<
        QString, std::function<void(const QJsonObject &, const QString &)>>
        moderationActionHandlers;

    std::unordered_map<
        QString, std::function<void(const QJsonObject &, const QString &)>>
        channelTermsActionHandlers;

    void handleResponse(const PubSubMessage &message);
    void handleListenResponse(const NonceInfo &info, bool failed);
    void handleUnlistenResponse(const NonceInfo &info, bool failed);
    void handleMessageResponse(const PubSubMessageMessage &message);

    // Register a nonce for a specific client
    void registerNonce(QString nonce, NonceInfo nonceInfo);

    // Find client associated with a nonce
    boost::optional<NonceInfo> findNonceInfo(QString nonce);

    std::unordered_map<QString, NonceInfo> nonces_;

    const QString host_;
    const PubSubClientOptions clientOptions_;

    bool stopping_{false};
};

}  // namespace chatterino
