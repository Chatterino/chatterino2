#pragma once

#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

#include <rapidjson/document.h>
#include <QString>
#include <pajlada/signals/signal.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct chatterinoconfig : public websocketpp::config::asio_tls_client {
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::elevel>
        elog_type;
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::alevel>
        alog_type;

    struct permessage_deflate_config {
    };

    typedef websocketpp::extensions::permessage_deflate::disabled<
        permessage_deflate_config>
        permessage_deflate_type;
};

using WebsocketClient = websocketpp::client<chatterinoconfig>;
using WebsocketHandle = websocketpp::connection_hdl;
using WebsocketErrorCode = websocketpp::lib::error_code;

#define MAX_PUBSUB_LISTENS 50
#define MAX_PUBSUB_CONNECTIONS 10

namespace detail {

    struct Listener {
        QString topic;
        bool authed;
        bool persistent;
        bool confirmed = false;
    };

    class PubSubClient : public std::enable_shared_from_this<PubSubClient>
    {
    public:
        PubSubClient(WebsocketClient &_websocketClient,
                     WebsocketHandle _handle);

        void start();
        void stop();

        bool listen(rapidjson::Document &message);
        void unlistenPrefix(const QString &prefix);

        void handlePong();

        bool isListeningToTopic(const QString &topic);

    private:
        void ping();
        bool send(const char *payload);

        WebsocketClient &websocketClient_;
        WebsocketHandle handle_;
        uint16_t numListens_ = 0;

        std::vector<Listener> listeners_;

        std::atomic<bool> awaitingPong_{false};
        std::atomic<bool> started_{false};
    };

}  // namespace detail

class PubSub
{
    using WebsocketMessagePtr =
        websocketpp::config::asio_tls_client::message_type::ptr;
    using WebsocketContextPtr =
        websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

    WebsocketClient websocketClient;
    std::unique_ptr<std::thread> mainThread;

public:
    PubSub();

    ~PubSub() = delete;

    enum class State {
        Connected,
        Disconnected,
    };

    void start();

    bool isConnected() const
    {
        return this->state == State::Connected;
    }

    pajlada::Signals::NoArgSignal connected;

    struct {
        struct {
            Signal<ClearChatAction> chatCleared;
            Signal<ModeChangedAction> modeChanged;
            Signal<ModerationStateAction> moderationStateChanged;

            Signal<BanAction> userBanned;
            Signal<UnbanAction> userUnbanned;

            Signal<AutomodAction> automodMessage;
            Signal<AutomodUserAction> automodUserMessage;
        } moderation;

        struct {
            // Parsing should be done in PubSubManager as well,
            // but for now we just send the raw data
            Signal<const rapidjson::Value &> received;
            Signal<const rapidjson::Value &> sent;
        } whisper;

        struct {
            Signal<rapidjson::Value &> redeemed;
        } pointReward;
    } signals_;

    void listenToWhispers(std::shared_ptr<TwitchAccount> account);

    void unlistenAllModerationActions();

    void listenToChannelModerationActions(
        const QString &channelID, std::shared_ptr<TwitchAccount> account);

    void listenToChannelPointRewards(const QString &channelID,
                                     std::shared_ptr<TwitchAccount> account);

    std::vector<std::unique_ptr<rapidjson::Document>> requests;

private:
    void listenToTopic(const QString &topic,
                       std::shared_ptr<TwitchAccount> account);

    void listen(rapidjson::Document &&msg);
    bool tryListen(rapidjson::Document &msg);

    bool isListeningToTopic(const QString &topic);

    void addClient();

    State state = State::Connected;

    std::map<WebsocketHandle, std::shared_ptr<detail::PubSubClient>,
             std::owner_less<WebsocketHandle>>
        clients;

    std::unordered_map<std::string, std::function<void(const rapidjson::Value &,
                                                       const QString &)>>
        moderationActionHandlers;

    void onMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr msg);
    void onConnectionOpen(websocketpp::connection_hdl hdl);
    void onConnectionClose(websocketpp::connection_hdl hdl);
    WebsocketContextPtr onTLSInit(websocketpp::connection_hdl hdl);

    void handleListenResponse(const rapidjson::Document &msg);
    void handleMessageResponse(const rapidjson::Value &data);

    void runThread();
};

}  // namespace chatterino
