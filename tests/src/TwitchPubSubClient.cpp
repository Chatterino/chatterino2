#include "mocks/BaseApplication.hpp"
#include "providers/twitch/PubSubClient.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "Test.hpp"

#include <QString>

#include <chrono>
#include <mutex>

using namespace chatterino;
using namespace std::chrono_literals;

/**
 * Server behaves normally and responds to pings (COMPLETE)
 * Server doesn't respond to pings, client should disconnect (COMPLETE)
 * Server randomly disconnects us, we should reconnect (COMPLETE)
 * Client listens to more than 50 topics, so it opens 2 connections (COMPLETE)
 * Server sends RECONNECT message to us, we should reconnect (INCOMPLETE, leaving for now since if we just ignore it and Twitch disconnects us we should already handle it properly)
 * Incoming ChannelPoints message
 **/

#define RUN_PUBSUB_TESTS

#ifdef RUN_PUBSUB_TESTS

namespace chatterino {

template <typename T>
class ReceivedMessage
{
    mutable std::mutex mutex;

    bool isSet{false};
    T t;

public:
    ReceivedMessage() = default;

    explicit operator bool() const
    {
        std::unique_lock lock(this->mutex);

        return this->isSet;
    }

    ReceivedMessage &operator=(const T &newT)
    {
        std::unique_lock lock(this->mutex);

        this->isSet = true;
        this->t = newT;

        return *this;
    }

    bool operator==(const T &otherT) const
    {
        std::unique_lock lock(this->mutex);

        return this->t == otherT;
    }

    const T *operator->() const
    {
        return &this->t;
    }
};

const QString TEST_SETTINGS = R"(
{
    "eventsub": {
        "enableExperimental": false
    }
}
)";

class FTest : public PubSub
{
public:
    explicit FTest(const char *path, std::chrono::seconds pingInterval)
        : PubSub(QString("wss://127.0.0.1:9050%1").arg(path), pingInterval)
    {
    }
};

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication(const char *path, std::chrono::seconds pingInterval)
        : mock::BaseApplication(TEST_SETTINGS)
        , pubSub(path, pingInterval)
    {
    }

    PubSub *getTwitchPubSub() override
    {
        return &this->pubSub;
    }

    FTest pubSub;
};

TEST(TwitchPubSubClient, ServerRespondsToPings)
{
    MockApplication a("", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    pubSub.listenToChannelPointRewards("123456");

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
    MockApplication a("/dont-respond-to-ping", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();
    pubSub.listenToChannelPointRewards("123456");

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
    MockApplication a("/disconnect-client-after-1s", 10s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 0);

    pubSub.listenToChannelPointRewards("123456");

    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);  // Listen RESPONSE & Pong
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    std::this_thread::sleep_for(950ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 2);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 1);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 2);
    ASSERT_EQ(pubSub.diag.messagesReceived, 4);  // new listen & new pong

    pubSub.stop();
}

TEST(TwitchPubSubClient, ExceedTopicLimit)
{
    MockApplication a("", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToChannelPointRewards(QString("1%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToChannelPointRewards(QString("2%1").arg(i));
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
    MockApplication a("", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    ASSERT_EQ(pubSub.diag.connectionsOpened, 0);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS * 2; ++i)
    {
        pubSub.listenToChannelPointRewards("123456");
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

}  // namespace chatterino

#endif
