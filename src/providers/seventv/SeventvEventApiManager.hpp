#pragma once

#include "providers/seventv/SeventvEventApiClient.hpp"
#include "providers/seventv/SeventvEventApiMessages.hpp"
#include "providers/seventv/SeventvEventApiWebsocket.hpp"
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
class SeventvEventApi
{
    using WebsocketMessagePtr =
        websocketpp::config::asio_tls_client::message_type::ptr;
    using WebsocketContextPtr =
        websocketpp::lib::shared_ptr<boost::asio::ssl::context>;

    template <typename T>
    using Signal =
        pajlada::Signals::Signal<T>;  // type-id is vector<T, Alloc<T>>

    eventapi::WebsocketClient websocketClient;
    std::unique_ptr<std::thread> mainThread;

public:
    // The max amount of connections we may open
    static constexpr int maxConnections = 10;

    SeventvEventApi(const QString &host);
    ~SeventvEventApi() = default;

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
        Signal<EventApiEmoteUpdate> emoteAdded;
        Signal<EventApiEmoteUpdate> emoteUpdated;
        Signal<EventApiEmoteUpdate> emoteRemoved;
    } signals_;

    void joinChannel(const QString &channelName);
    void partChannel(const QString &channelName);
    bool isJoinedChannel(const QString &channelName);

private:
    std::vector<QString> pendingChannels;
    std::atomic<bool> addingClient{false};
    ExponentialBackoff<5> connectBackoff{std::chrono::milliseconds(1000)};

    State state = State::Connected;

    std::map<eventapi::WebsocketHandle, std::shared_ptr<SeventvEventApiClient>,
             std::owner_less<eventapi::WebsocketHandle>>
        clients;

    void onMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr msg);
    void onConnectionOpen(websocketpp::connection_hdl hdl);
    void onConnectionFail(websocketpp::connection_hdl hdl);
    void onConnectionClose(websocketpp::connection_hdl hdl);
    WebsocketContextPtr onTLSInit(websocketpp::connection_hdl hdl);

    void runThread();
    void addClient();

    bool tryJoinChannel(const QString &channel);
    void handleUpdateAction(const EventApiEmoteUpdate &update);

    std::shared_ptr<boost::asio::io_service::work> work{nullptr};

    const QString host_;

    bool stopping_{false};
};
}  // namespace chatterino
