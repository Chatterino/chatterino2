#pragma once

#include "providers/seventv/SeventvEventApi.hpp"
#include "providers/seventv/SeventvEventApiClient.hpp"
#include "providers/seventv/SeventvEventApiWebsocket.hpp"
#include "providers/seventv/eventapimessages/SeventvEventApiDispatch.hpp"
#include "providers/seventv/eventapimessages/SeventvEventApiMessage.hpp"
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
        return this->state_ == State::Connected;
    }

    struct {
        Signal<SeventvEventApiEmoteAddDispatch> emoteAdded;
        Signal<SeventvEventApiEmoteUpdateDispatch> emoteUpdated;
        Signal<SeventvEventApiEmoteRemoveDispatch> emoteRemoved;
        Signal<SeventvEventApiUserConnectionUpdateDispatch> userUpdated;
    } signals_;

    void subscribeUser(const QString &userId, const QString &emoteSetId);
    void unsubscribeUser(const QString &id);
    void unsubscribeEmoteSet(const QString &id);

private:
    std::vector<SeventvEventApiSubscription> pendingSubscriptions_;
    std::unordered_set<QString> subscribedEmoteSets_;
    std::unordered_set<QString> subscribedUsers_;
    std::atomic<bool> addingClient_{false};
    ExponentialBackoff<5> connectBackoff_{std::chrono::milliseconds(1000)};

    State state_ = State::Connected;

    std::map<eventapi::WebsocketHandle, std::shared_ptr<SeventvEventApiClient>,
             std::owner_less<eventapi::WebsocketHandle>>
        clients_;

    void onMessage(websocketpp::connection_hdl hdl, WebsocketMessagePtr msg);
    void onConnectionOpen(websocketpp::connection_hdl hdl);
    void onConnectionFail(websocketpp::connection_hdl hdl);
    void onConnectionClose(websocketpp::connection_hdl hdl);
    WebsocketContextPtr onTLSInit(websocketpp::connection_hdl hdl);

    void runThread();
    void addClient();

    void subscribe(const SeventvEventApiSubscription &subscription);
    bool trySubscribe(const SeventvEventApiSubscription &subscription);
    void unsubscribe(const SeventvEventApiSubscription &subscription);
    void handleDispatch(const SeventvEventApiDispatch &dispatch);

    std::shared_ptr<boost::asio::io_service::work> work_{nullptr};

    const QString host_;

    bool stopping_{false};
};
}  // namespace chatterino
