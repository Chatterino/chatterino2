#pragma once

#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/functional/hash.hpp>
#include <QJsonObject>
#include <QString>

#include <memory>
#include <string>
#include <thread>

namespace chatterino::eventsub {

class EventSubClient final : public lib::Listener
{
public:
    void onSessionWelcome(
        lib::messages::Metadata metadata,
        lib::payload::session_welcome::Payload payload) override;

    void onNotification(lib::messages::Metadata metadata,
                        const boost::json::value &jv) override;

    void onChannelBan(lib::messages::Metadata metadata,
                      lib::payload::channel_ban::v1::Payload payload) override;

    void onStreamOnline(
        lib::messages::Metadata metadata,
        lib::payload::stream_online::v1::Payload payload) override;

    void onStreamOffline(
        lib::messages::Metadata metadata,
        lib::payload::stream_offline::v1::Payload payload) override;

    void onChannelChatNotification(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_notification::v1::Payload payload) override;

    void onChannelUpdate(
        lib::messages::Metadata metadata,
        lib::payload::channel_update::v1::Payload payload) override;

    void onChannelChatMessage(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_message::v1::Payload payload) override;

    QString getSessionID() const
    {
        return this->sessionID;
    }

private:
    QString sessionID;
};

class Controller
{
public:
    Controller();
    ~Controller();

    /// Subscribe will make a request to each open connection and ask them to
    /// add this subscription.
    ///
    /// If this subscription already exists, this call is a no-op.
    ///
    /// If no open connection has room for this subscription, this function will
    /// create a new connection and queue up the subscription to run again after X seconds.
    ///
    /// TODO: Return a SubscriptionHandle that handles unsubscriptions
    /// Dupe subscriptions should return shared subscription handles
    /// So no more owners of the subscription handle means we send an unsubscribe request
    void subscribe(const SubscriptionRequest &request, bool isQueued = false);

private:
    void createConnection();
    void registerConnection(std::weak_ptr<lib::Session> &&connection);

    const std::string userAgent;

    std::string eventSubHost;
    std::string eventSubPort;
    std::string eventSubPath;

    std::unique_ptr<std::thread> thread;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::weak_ptr<lib::Session>> connections;

    std::unordered_map<SubscriptionRequest,
                       std::unique_ptr<boost::asio::deadline_timer>>
        queuedSubscriptions;
};

}  // namespace chatterino::eventsub
