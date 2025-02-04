#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/json.hpp>
#include <qdebug.h>
#include <qlogging.h>

#include <iostream>
#include <memory>

using namespace chatterino::eventsub;

class MyListener final : public lib::Listener
{
public:
    void onSessionWelcome(
        lib::messages::Metadata metadata,
        lib::payload::session_welcome::Payload payload) override
    {
        (void)metadata;
        std::cout << "ON session welcome " << payload.id << " XD\n";
    }

    void onNotification(lib::messages::Metadata metadata,
                        const boost::json::value &jv) override
    {
        (void)metadata;
        std::cout << "on notification: " << jv << '\n';
    }

    void onChannelBan(lib::messages::Metadata metadata,
                      lib::payload::channel_ban::v1::Payload payload) override
    {
        (void)metadata;
        std::cout << "Channel ban occured in "
                  << payload.event.broadcasterUserLogin << "'s channel:"
                  << " isPermanent=" << payload.event.isPermanent
                  << " reason=" << payload.event.reason
                  << " userLogin=" << payload.event.userLogin
                  << " moderatorLogin=" << payload.event.moderatorUserLogin
                  << '\n';
    }

    void onStreamOnline(
        lib::messages::Metadata metadata,
        lib::payload::stream_online::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "ON STREAM ONLINE XD\n";
    }

    void onStreamOffline(
        lib::messages::Metadata metadata,
        lib::payload::stream_offline::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "ON STREAM OFFLINE XD\n";
    }

    void onChannelChatNotification(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_notification::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Received channel.chat.notification v1\n";
    }

    void onChannelUpdate(
        lib::messages::Metadata metadata,
        lib::payload::channel_update::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Channel update event!\n";
    }

    void onChannelChatMessage(
        lib::messages::Metadata metadata,
        lib::payload::channel_chat_message::v1::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Channel chat message event!\n";
    }

    void onChannelModerate(
        lib::messages::Metadata metadata,
        lib::payload::channel_moderate::v2::Payload payload) override
    {
        (void)metadata;
        (void)payload;
        std::cout << "Channel moderate event!\n";
    }

    // Add your new subscription types above this line
};

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    lib::String a("foo");

    // qDebug() << "xd1:"
    //          << QString::fromStdString(std::get<std::string>(a.backingString));
    qDebug() << "xd2:" << a.qt();
    qDebug() << "xd3:" << a.qt();

    std::string userAgent{"chatterino-eventsub-testing"};

    // for use with twitch CLI: twitch event websocket start-server --ssl --port 3012
    // std::string host{"localhost"};
    // std::string port{"3012"};
    // std::string path{"/ws"};

    // for use with websocat: websocat -s 8080 --pkcs12-der certificate.p12
    std::string host{"localhost"};
    std::string port{"8080"};
    std::string path;

    // for use with real Twitch eventsub
    // std::string host{"eventsub.wss.twitch.tv"};
    // std::string port("443");
    // std::string path("/ws");

    try
    {
        boost::asio::io_context ctx(1);

        boost::asio::ssl::context sslContext{
            boost::asio::ssl::context::tlsv12_client};

        // TODO: Load certificates into SSL context

        std::make_shared<lib::Session>(ctx, sslContext,
                                       std::make_unique<MyListener>())
            ->run(host, port, path, userAgent);

        ctx.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
