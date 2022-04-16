#include "providers/twitch/PubsubManager.hpp"

#include <gtest/gtest.h>

using namespace chatterino;
using namespace std::chrono_literals;

/**
 * Server behaves normally and responds to pings (COMPLETE)
 * Server doesn't respond to pings, client should disconnect (COMPLETE)
 * Server randomly disconnects us, we should reconnect (COMPLETE)
 * Client listens to more than 50 topics, so it opens 2 connections (COMPLETE)
 * Server sends RECONNECT message to us, we should reconnect (INCOMPLETE)
 * Listen that required authentication, but authentication is missing
 * Listen that required authentication, but authentication is wrong
 **/

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
    ASSERT_EQ(pubSub->diag.messagesReceived, 1);

    std::this_thread::sleep_for(2s);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 3);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 3);
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
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    pubSub->stop();

    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 2);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);
}

TEST(TwitchPubSubClient, RandomDisconnect)
{
    auto pingInterval = std::chrono::seconds(1);
    const QString host("wss://127.0.0.1:9050/disconnect-client-after-1s");

    auto *pubSub = new PubSub(host, pingInterval);
    pubSub->setAccountData("token", "123456");
    pubSub->start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 0);

    pubSub->listenToTopic("test");

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 1);

    std::this_thread::sleep_for(750ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);

    std::this_thread::sleep_for(750ms);

    ASSERT_EQ(pubSub->diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub->diag.connectionsOpened, 2);

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

    for (auto i = 0; i < PubSubClient::listensPerConnection; ++i)
    {
        pubSub->listenToTopic("test");
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub->diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub->diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub->diag.connectionsFailed, 0);

    for (auto i = 0; i < PubSubClient::listensPerConnection; ++i)
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

    for (auto i = 0; i < PubSubClient::listensPerConnection * 2; ++i)
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
    ASSERT_EQ(pubSub->diag.messagesReceived, 2);

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

#endif
