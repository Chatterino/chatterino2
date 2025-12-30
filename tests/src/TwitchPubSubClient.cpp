#include "mocks/BaseApplication.hpp"
#include "providers/twitch/PubSubClient.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "Test.hpp"

#include <QString>
#include <QtCore/qtestsupport_core.h>

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

    QTest::qWait(50);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    pubSub.listenToChannelPointRewards("123456");

    QTest::qWait(150);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 1);  // LISTEN
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    QTest::qWait(2 * 1000);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 3);  // LISTEN + 2 * PONG

    pubSub.stop();

    // after exactly one event loop iteration, we should see updated counters
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 3);
    ASSERT_EQ(pubSub.diag.listenResponses, 1);
}

TEST(TwitchPubSubClient, ServerDoesntRespondToPings)
{
    MockApplication a("/dont-respond-to-ping", 1s);
    auto &pubSub = a.pubSub;

    pubSub.listenToChannelPointRewards("123456");

    QTest::qWait(750);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 1);

    QTest::qWait(1500);  // we need to wait for two rounds of pings

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);

    pubSub.stop();

    // after exactly one event loop iteration, we should see updated counters
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);
}

TEST(TwitchPubSubClient, DisconnectedAfter1s)
{
    MockApplication a("/disconnect-client-after-1s", 10s);
    auto &pubSub = a.pubSub;

    QTest::qWait(50);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 0);

    pubSub.listenToChannelPointRewards("123456");

    QTest::qWait(500);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 1);  // Listen RESPONSE
    ASSERT_EQ(pubSub.diag.listenResponses, 1);

    QTest::qWait(950);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.listenResponses, 2);
    ASSERT_EQ(pubSub.diag.messagesReceived, 2);  // new listen

    pubSub.stop();
}

TEST(TwitchPubSubClient, ExceedTopicLimit)
{
    MockApplication a("", 1s);
    auto &pubSub = a.pubSub;

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
    ASSERT_EQ(pubSub.diag.messagesReceived, 0);

    for (size_t i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToChannelPointRewards(QString("1%1").arg(i));
    }

    QTest::qWait(100);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 1);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);

    for (size_t i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToChannelPointRewards(QString("2%1").arg(i));
    }

    QTest::qWait(200);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 0);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);

    pubSub.stop();

    // after exactly one event loop iteration, we should see updated counters
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    ASSERT_EQ(pubSub.wsDiag().connectionsOpened, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsClosed, 2);
    ASSERT_EQ(pubSub.wsDiag().connectionsFailed, 0);
}

}  // namespace chatterino

#endif
