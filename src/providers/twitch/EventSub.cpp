#include "providers/twitch/EventSub.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/PostToThread.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/json.hpp>
#include <eventsub/listener.hpp>
#include <eventsub/session.hpp>

#include <chrono>
#include <memory>

using namespace std::literals::chrono_literals;

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoTwitchEventSub;

}  // namespace

namespace chatterino {

class MyListener final : public eventsub::Listener
{
public:
    void onSessionWelcome(
        eventsub::messages::Metadata metadata,
        eventsub::payload::session_welcome::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On session welcome:" << payload.id.c_str();

        auto sessionID = QString::fromStdString(payload.id);

        const auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            return;
        }

        auto sourceUserID = currentUser->getUserId();

        getApp()->twitch->forEachChannelAndSpecialChannels(
            [sessionID, sourceUserID](const ChannelPtr &channel) {
                if (channel->getType() == Channel::Type::Twitch)
                {
                    auto *twitchChannel =
                        dynamic_cast<TwitchChannel *>(channel.get());

                    auto roomID = twitchChannel->roomId();

                    if (channel->isBroadcaster())
                    {
                        QJsonObject condition;
                        condition.insert("broadcaster_user_id", roomID);

                        getHelix()->createEventSubSubscription(
                            "channel.ban", "1", sessionID, condition,
                            [roomID](const auto &response) {
                                qDebug() << "Successfully subscribed to "
                                            "channel.ban in"
                                         << roomID << ":" << response;
                            },
                            [roomID](auto error, const auto &message) {
                                (void)error;
                                qDebug()
                                    << "Failed subscription to channel.ban in"
                                    << roomID << ":" << message;
                            });
                    }

                    {
                        QJsonObject condition;
                        condition.insert("broadcaster_user_id", roomID);
                        condition.insert("user_id", sourceUserID);

                        getHelix()->createEventSubSubscription(
                            "channel.chat.notification", "1", sessionID,
                            condition,
                            [roomID](const auto &response) {
                                qDebug() << "Successfully subscribed to "
                                            "channel.chat.notification in "
                                         << roomID << ":" << response;
                            },
                            [roomID](auto error, const auto &message) {
                                (void)error;
                                qDebug() << "Failed subscription to "
                                            "channel.chat.notification in"
                                         << roomID << ":" << message;
                            });
                    }
                }
            });
    }

    void onNotification(eventsub::messages::Metadata metadata,
                        const boost::json::value &jv) override
    {
        (void)metadata;
        auto jsonString = boost::json::serialize(jv);
        qCDebug(LOG) << "on notification: " << jsonString.c_str();
    }

    void onChannelBan(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_ban::v1::Payload payload) override
    {
        (void)metadata;

        auto roomID = QString::fromStdString(payload.event.broadcasterUserID);

        BanAction action{};

        action.timestamp = std::chrono::steady_clock::now();
        action.roomID = roomID;
        action.source = ActionUser{
            .id = QString::fromStdString(payload.event.moderatorUserID),
            .login = QString::fromStdString(payload.event.moderatorUserLogin),
            .displayName =
                QString::fromStdString(payload.event.moderatorUserName),
        };
        action.target = ActionUser{
            .id = QString::fromStdString(payload.event.userID),
            .login = QString::fromStdString(payload.event.userLogin),
            .displayName = QString::fromStdString(payload.event.userName),
        };
        action.reason = QString::fromStdString(payload.event.reason);
        if (payload.event.isPermanent)
        {
            action.duration = 0;
        }
        else
        {
            auto timeoutDuration = payload.event.timeoutDuration();
            auto timeoutDurationInSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(
                    timeoutDuration)
                    .count();
            action.duration = timeoutDurationInSeconds;
        }

        auto chan = getApp()->twitch->getChannelOrEmptyByID(roomID);

        runInGuiThread([action{std::move(action)}, chan{std::move(chan)}] {
            MessageBuilder msg(action);
            msg->flags.set(MessageFlag::PubSub);
            chan->addOrReplaceTimeout(msg.release());
        });
    }

    void onStreamOnline(
        eventsub::messages::Metadata metadata,
        eventsub::payload::stream_online::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On stream online event for channel"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onStreamOffline(
        eventsub::messages::Metadata metadata,
        eventsub::payload::stream_offline::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On stream offline event for channel"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onChannelChatNotification(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_chat_notification::v1::Payload payload)
        override
    {
        (void)metadata;
        qCDebug(LOG) << "On channel chat notification for"
                     << payload.event.broadcasterUserLogin.c_str();
    }

    void onChannelUpdate(
        eventsub::messages::Metadata metadata,
        eventsub::payload::channel_update::v1::Payload payload) override
    {
        (void)metadata;
        qCDebug(LOG) << "On channel update for"
                     << payload.event.broadcasterUserLogin.c_str();
    }
};

void EventSub::start()
{
    const auto userAgent = QStringLiteral("chatterino/%1 (%2)")
                               .arg(Version::instance().version(),
                                    Version::instance().commitHash())
                               .toUtf8()
                               .toStdString();

    // for use with twitch CLI: twitch event websocket start-server --ssl --port 3012
    // std::string host{"localhost"};
    // std::string port{"3012"};
    // std::string path{"/ws"};

    // for use with websocat: websocat -s 8080 --pkcs12-der certificate.p12
    // std::string host{"localhost"};
    // std::string port{"8080"};
    // std::string path;

    // for use with real Twitch eventsub
    std::string host{"eventsub.wss.twitch.tv"};
    std::string port{"443"};
    std::string path{"/ws"};

    this->mainThread = std::make_unique<std::thread>([=] {
        try
        {
            boost::asio::io_context ctx(1);

            boost::asio::ssl::context sslContext{
                boost::asio::ssl::context::tlsv12_client};

            // TODO: Load certificates into SSL context

            std::make_shared<eventsub::Session>(ctx, sslContext,
                                                std::make_unique<MyListener>())
                ->run(host, port, path, userAgent);

            ctx.run();
        }
        catch (std::exception &e)
        {
            qCWarning(LOG) << "Error in EventSub run thread" << e.what();
        }
    });
}

}  // namespace chatterino
