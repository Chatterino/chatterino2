#include "mocks/BaseApplication.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubClient.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/pubsubmessages/AutoMod.hpp"
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
 * Listen that required authentication, but authentication is missing (COMPLETE)
 * Listen that required authentication, but authentication is wrong (COMPLETE)
 * Incoming AutoMod message
 * Incoming ChannelPoints message
 * Incoming ChatModeratorAction message (COMPLETE)
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
    explicit FTest(const char *path, std::chrono::seconds pingInterval,
                   QString token = "token")
        : PubSub(QString("wss://127.0.0.1:9050%1").arg(path), pingInterval)
    {
        auto account = std::make_shared<TwitchAccount>("testaccount_420", token,
                                                       "clientid", "123456");
        this->setAccount(account);
    }
};

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication(const char *path, std::chrono::seconds pingInterval,
                    QString token = "token")
        : mock::BaseApplication(TEST_SETTINGS)
        , pubSub(path, pingInterval, token)
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

    pubSub.listenToChannelModerationActions("123456");

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
    pubSub.listenToChannelModerationActions("123456");

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

    pubSub.listenToChannelModerationActions("123456");

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
        pubSub.listenToChannelModerationActions(QString("1%1").arg(i));
    }

    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(pubSub.diag.connectionsOpened, 1);
    ASSERT_EQ(pubSub.diag.connectionsClosed, 0);
    ASSERT_EQ(pubSub.diag.connectionsFailed, 0);

    for (auto i = 0; i < PubSubClient::MAX_LISTENS; ++i)
    {
        pubSub.listenToChannelModerationActions(QString("2%1").arg(i));
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
        pubSub.listenToChannelModerationActions("123456");
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

TEST(TwitchPubSubClient, ModeratorActionsUserBanned)
{
    MockApplication a("/moderator-actions-user-banned", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    ReceivedMessage<BanAction> received;

    std::ignore =
        pubSub.moderation.userBanned.connect([&received](const auto &action) {
            received = action;
        });

    ASSERT_EQ(pubSub.diag.listenResponses, 0);

    pubSub.listenToChannelModerationActions("123456");

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
    MockApplication a("/authentication-required", 1s, "");
    auto &pubSub = a.pubSub;

    pubSub.start();

    pubSub.listenToChannelModerationActions("123456");

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
    MockApplication a("/authentication-required", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    pubSub.listenToChannelModerationActions("123456");

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
    MockApplication a("/authentication-required", 1s, "xD");
    auto &pubSub = a.pubSub;

    pubSub.start();

    pubSub.listenToChannelModerationActions("123456");

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
    MockApplication a("/automod-held", 1s);
    auto &pubSub = a.pubSub;

    pubSub.start();

    ReceivedMessage<PubSubAutoModQueueMessage> received;
    ReceivedMessage<QString> channelID;

    std::ignore = pubSub.moderation.autoModMessageCaught.connect(
        [&](const auto &msg, const QString &incomingChannelID) {
            received = msg;
            channelID = incomingChannelID;
        });

    pubSub.listenToAutomod("117166826");

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

}  // namespace chatterino

#endif
