#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubClient.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/pubsubmessages/AutoMod.hpp"
#include "providers/twitch/pubsubmessages/Whisper.hpp"
#include "TestHelpers.hpp"

#include <gtest/gtest.h>
#include <QString>

#include <chrono>
#include <optional>

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

class FTest : public PubSub
{
public:
    explicit FTest(const char *path, std::chrono::seconds pingInterval)
        : PubSub(QString("wss://127.0.0.1:9050%1").arg(path), pingInterval)
    {
    }
};

TEST(TwitchPubSubClient, ServerRespondsToPings)
{
    FTest pubSub("", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    pubSub.listenToTopic("test");

    std::this_thread::sleep_for(150ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    std::this_thread::sleep_for(2s);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 4);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 4);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);
}

TEST(TwitchPubSubClient, ServerDoesntRespondToPings)
{
    FTest pubSub("/dont-respond-to-ping", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();
    pubSub.listenToTopic("test");

    std::this_thread::sleep_for(750ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 1);

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
}

TEST(TwitchPubSubClient, DisconnectedAfter1s)
{
    FTest pubSub("/disconnect-client-after-1s", 10s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 0);

    pubSub.listenToTopic("test");

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);  // Listen RESPONSE & Pong
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    std::this_thread::sleep_for(350ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);

    std::this_thread::sleep_for(600ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 2);
    ASSERT_EQ(pubSub.diag.messagesReceived, 4);  // new listen & new pong

    pubSub.stop();
}

TEST(TwitchPubSubClient, ExceedTopicLimit)
{
    FTest pubSub("", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToTopic(QString("test-1.%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToTopic(QString("test-2.%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ExceedTopicLimitSingleStep)
{
    FTest pubSub("", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS * 2; ++i)
    {
        pubSub.listenToTopic("test");
    }

    std::this_thread::sleep_for(150ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ReceivedWhisper)
{
    FTest pubSub("/receive-whisper", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    ReceivedMessage<PubSubWhisperMessage> aReceivedWhisper;

    pubSub.signals_.whisper.received.connect(
        [&aReceivedWhisper](const auto &whisperMessage) {
            aReceivedWhisper = whisperMessage;
        });

    pubSub.listenToTopic("whispers.123456");

    std::this_thread::sleep_for(150ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 3);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    ASSERT_TRUE(aReceivedWhisper);

    ASSERT_EQ(aReceivedWhisper->body, QString("me Kappa"));
    ASSERT_EQ(aReceivedWhisper->fromUserLogin, QString("pajbot"));
    ASSERT_EQ(aReceivedWhisper->fromUserID, QString("82008718"));

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, ModeratorActionsUserBanned)
{
    FTest pubSub("/moderator-actions-user-banned", 1s);

    pubSub.setAccountData("token", "123456");
    pubSub.start();

    ReceivedMessage<BanAction> received;

    pubSub.signals_.moderation.userBanned.connect(
        [&received](const auto &action) {
            received = action;
        });

    ASSERT_EQ(pubSub.diag.listenResponses, 0);

    pubSub.listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 3);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    ASSERT_TRUE(received);

    ActionUser expectedTarget{"140114344", "1xelerate", "", QColor()};
    ActionUser expectedSource{"117691339", "mm2pl", "", QColor()};

    ASSERT_EQ(received->reason, QString());
    ASSERT_EQ(received->duration, 0);
    ASSERT_EQ(received->target, expectedTarget);
    ASSERT_EQ(received->source, expectedSource);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, MissingToken)
{
    // The token that's required is "xD"
    FTest pubSub("/authentication-required", 1s);

    // pubSub.setAccountData("", "123456");
    pubSub.start();

    pubSub.listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(150ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
    ASSERT_EQ(pubSub.diag.listenResponses, 0);
    ASSERT_EQ(pubSub.diag.failedListenResponses, 1);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, WrongToken)
{
    // The token that's required is "xD"
    FTest pubSub("/authentication-required", 1s);

    pubSub.setAccountData("wrongtoken", "123456");
    pubSub.start();

    pubSub.listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
    ASSERT_EQ(pubSub.diag.listenResponses, 0);
    ASSERT_EQ(pubSub.diag.failedListenResponses, 1);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, CorrectToken)
{
    // The token that's required is "xD"
    FTest pubSub("/authentication-required", 1s);

    pubSub.setAccountData("xD", "123456");
    pubSub.start();

    pubSub.listenToTopic("chat_moderator_actions.123456.123456");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);
    ASSERT_EQ(pubSub.diag.failedListenResponses, 0);

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

TEST(TwitchPubSubClient, AutoModMessageHeld)
{
    FTest pubSub("/automod-held", 1s);

    pubSub.setAccountData("xD", "123456");
    pubSub.start();

    ReceivedMessage<PubSubAutoModQueueMessage> received;
    ReceivedMessage<QString> channelID;

    pubSub.signals_.moderation.autoModMessageCaught.connect(
        [&](const auto &msg, const QString &incomingChannelID) {
            received = msg;
            channelID = incomingChannelID;
        });

    pubSub.listenToTopic("automod-queue.117166826.117166826");

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 3);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);
    ASSERT_EQ(pubSub.diag.failedListenResponses, 0);

    ASSERT_TRUE(received);
    ASSERT_TRUE(channelID);

    ASSERT_EQ(channelID, "117166826");
    ASSERT_EQ(received->messageText, "kurwa");

    pubSub.stop();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
}

#endif
