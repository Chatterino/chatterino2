#include "providers/twitch/PubSubManager.hpp"

#include "providers/twitch/PubSubActions.hpp"

#include <gtest/gtest.h>

using namespace chatterino;
using namespace std::chrono_literals;

/**
 * Server behaves normally and responds to pings (COMPLETE)
 * Server doesn't respond to pings, client should disconnect (COMPLETE)
 * Server randomly disconnects us, we should reconnect (COMPLETE)
 * Client listens to more than 50 topics, so it opens 2 connections (COMPLETE)
 * Server sends RECONNECT message to us, we should reconnect (INCOMPLETE, leaving for now since if we just ignore it and Twitch disconnects us we should already handle it properly)
 * Listen that required authentication, but authentication is missing (COMPLETE)
 * Listen that required authentication, but authentication is wrong (COMPLETE)
 * Incoming Whisper message (COMPLETE)
 * Incoming AutoMod message
 * Incoming ChannelPoints message
 * Incoming ChatModeratorAction message (COMPLETE)
 **/

#define RUN_PUBSUB_TESTS

#ifdef RUN_PUBSUB_TESTS

TEST(TwitchPubSubClient, ServerRespondsToPings)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    pubSub->listenToTopic("test");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);

    std::this_thread::sleep_for(2s);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 4);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 4);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);
}

TEST(TwitchPubSubClient, ServerDoesntRespondToPings)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050/dont-respond-to-ping");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();
    pubSub->listenToTopic("test");

    std::this_thread::sleep_for(750ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 1);

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);
}

TEST(TwitchPubSubClient, DisconnectedAfter1s)
{
    auto pingInterval = std::chrono::seconds(10);
    const QString host("wss://127.0.0.1:9050/disconnect-client-after-1s");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);
    ASSERT_EQ(pubSub->diag.listenResponses, 0);

    pubSub->listenToTopic("test");

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);  // Listen RESPONSE & Pong
    ASSERT_EQ(pubSub->diag.listenResponses, 1);

    std::this_thread::sleep_for(350ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);

    std::this_thread::sleep_for(600ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.listenResponses, 2);
    ASSERT_EQ(pubSub->diag.messagesReceived, 4);  // new listen & new pong

    pubSub->stop();
}

TEST(TwitchPubSubClient, ExceedTopicLimit)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub->listenToTopic(QString("test-1.%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub->listenToTopic(QString("test-2.%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ExceedTopicLimitSingleStep)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS * 2; ++i)
    {
        pubSub->listenToTopic("test");
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ReceivedWhisper)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050/receive-whisper");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    boost::optional<PubSubWhisperMessage> oReceivedWhisper;

    pubSub->signals_.whisper.received.connect(
        [&oReceivedWhisper](const auto &whisperMessage) {
            oReceivedWhisper = whisperMessage;
        });

    pubSub->listenToTopic("whispers.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 3);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);

    ASSERT_TRUE(oReceivedWhisper);

    auto receivedWhisper = *oReceivedWhisper;

    ASSERT_EQ(receivedWhisper.body, QString("me Kappa"));
    ASSERT_EQ(receivedWhisper.fromUserLogin, QString("pajbot"));
    ASSERT_EQ(receivedWhisper.fromUserID, QString("82008718"));

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ModeratorActionsUserBanned)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050/moderator-actions-user-banned");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    boost::optional<BanAction> oReceivedAction;

    pubSub->signals_.moderation.userBanned.connect(
        [&oReceivedAction](const auto &action) {
            oReceivedAction = action;
        });

    ASSERT_EQ(pubSub->diag.listenResponses, 0);

    pubSub->listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 3);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);

    ASSERT_TRUE(oReceivedAction);

    auto receivedAction = *oReceivedAction;

    ActionUser expectedTarget{"140114344", "1xelerate", "", QColor()};
    ActionUser expectedSource{"117691339", "mm2pl", "", QColor()};

    ASSERT_EQ(receivedAction.reason, QString());
    ASSERT_EQ(receivedAction.duration, 0);
    ASSERT_EQ(receivedAction.target, expectedTarget);
    ASSERT_EQ(receivedAction.source, expectedSource);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, MissingToken)
{
    auto pingInterval = std::chrono::seconds(1);
    // The token that's required is "xD"
    const QString host("wss://127.0.0.1:9050/authentication-required");

    auto *pubSub = new PubSub(host, pingInterval);
    // pubSub->setAccountData("", "123456");
    pubSub->start();

    pubSub->listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);
    ASSERT_EQ(pubSub->diag.listenResponses, 0);
    ASSERT_EQ(pubSub->diag.failedListenResponses, 1);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, WrongToken)
{
    auto pingInterval = std::chrono::seconds(1);
    // The token that's required is "xD"
    const QString host("wss://127.0.0.1:9050/authentication-required");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("wrongtoken", "123456");
    pubSub->start();

    pubSub->listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);
    ASSERT_EQ(pubSub->diag.listenResponses, 0);
    ASSERT_EQ(pubSub->diag.failedListenResponses, 1);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, CorrectToken)
{
    auto pingInterval = std::chrono::seconds(1);
    // The token that's required is "xD"
    const QString host("wss://127.0.0.1:9050/authentication-required");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("xD", "123456");
    pubSub->start();

    pubSub->listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);
    ASSERT_EQ(pubSub->diag.failedListenResponses, 0);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, AutoModMessageHeld)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050/automod-held");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("xD", "123456");
    pubSub->start();

    boost::optional<PubSubAutoModQueueMessage> oReceived;
    boost::optional<QString> oChannelID;

    pubSub->signals_.moderation.autoModMessageCaught.connect(
        [&](const auto &msg, const QString &channelID) {
            oReceived = msg;
            oChannelID = channelID;
        });

    pubSub->listenToTopic("automod-queue.117166826.117166826");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 3);
    ASSERT_EQ(pubSub->diag.listenResponses, 1);
    ASSERT_EQ(pubSub->diag.failedListenResponses, 0);

    ASSERT_TRUE(oReceived);
    ASSERT_TRUE(oChannelID);

    auto received = *oReceived;
    auto channelID = *oChannelID;

    ASSERT_EQ(channelID, "117166826");
    ASSERT_EQ(received.messageText, "kurwa");

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
}

#endif
