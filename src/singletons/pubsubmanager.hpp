#pragma once

#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/helper/pubsubactions.hpp"

#include <rapidjson/document.h>
#include <QString>
#include <pajlada/signals/signal.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace chatterino {
namespace singletons {

using WebsocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WebsocketHandle = websocketpp::connection_hdl;
using WebsocketErrorCode = websocketpp::lib::error_code;

#define MAX_PUBSUB_LISTENS 50
#define MAX_PUBSUB_CONNECTIONS 10

struct Listener {
    std::string topic;
    bool authed;
    bool persistent;
    bool confirmed = false;
};

class PubSubClient : public std::enable_shared_from_this<PubSubClient>
{
    WebsocketClient &websocketClient;
    WebsocketHandle handle;
    uint16_t numListens = 0;

    std::vector<Listener> listeners;

    std::atomic<bool> awaitingPong{false};
    std::atomic<bool> started{false};

public:
    PubSubClient(WebsocketClient &_websocketClient, WebsocketHandle _handle);

    void Start();
    void Stop();

    bool Listen(rapidjson::Document &message);
    void UnlistenPrefix(const std::string &prefix);

    void HandlePong();

    bool isListeningToTopic(const std::string &topic);

private:
    void Ping();
    bool Send(const char *payload);
};

class PubSubManager
{
    PubSubManager();
    friend class Application;

    using WebsocketMessagePtr = websocketpp::config::asio_tls_client::message_type::ptr;
    using WebsocketContextPtr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

    template <typename T>
    using Signal = pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

    WebsocketClient websocketClient;
    std::unique_ptr<std::thread> mainThread;

public:
    ~PubSubManager() = delete;

    enum class State {
        Connected,
        Disconnected,
    };

    void Start();

    bool IsConnected() const
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
        } moderation;

        struct {
            // Parsing should be done in PubSubManager as well,
            // but for now we just send the raw data
            Signal<const rapidjson::Value &> received;
            Signal<const rapidjson::Value &> sent;
        } whisper;
    } sig;

    void ListenToWhispers(std::shared_ptr<providers::twitch::TwitchAccount> account);

    void UnlistenAllModerationActions();

    void ListenToChannelModerationActions(
        const QString &channelID, std::shared_ptr<providers::twitch::TwitchAccount> account);

    std::vector<std::unique_ptr<rapidjson::Document>> requests;

private:
    void listenToTopic(const std::string &topic,
                       std::shared_ptr<providers::twitch::TwitchAccount> account);

    void Listen(rapidjson::Document &&msg);
    bool TryListen(rapidjson::Document &msg);

    bool isListeningToTopic(const std::string &topic);

    void AddClient();

    State state = State::Connected;

    std::map<WebsocketHandle, std::shared_ptr<PubSubClient>, std::owner_less<WebsocketHandle>>
        clients;

    std::unordered_map<std::string, std::function<void(const rapidjson::Value &, const QString &)>>
        moderationActionHandlers;

    void OnMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr msg);
    void OnConnectionOpen(websocketpp::connection_hdl hdl);
    void OnConnectionClose(websocketpp::connection_hdl hdl);
    WebsocketContextPtr OnTLSInit(websocketpp::connection_hdl hdl);

    void HandleListenResponse(const rapidjson::Document &msg);
    void HandleMessageResponse(const rapidjson::Value &data);

    void RunThread();
};

}  // namespace singletons
}  // namespace chatterino
