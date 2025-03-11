#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "lib/Snapshot.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/twitch/eventsub/Connection.hpp"
#include "Test.hpp"
#include "util/QCompareCaseInsensitive.hpp"

#include <QString>

using namespace chatterino;
using namespace literals;

namespace {

/// Controls whether snapshots will be updated (true) or verified (false)
///
/// In CI, all snapshots must be verified, thus the integrity tests checks for
/// this constant.
///
/// When adding a test, start with `{ "input": {...} }` and set this to `true`
/// to generate an initial snapshot. Make sure to verify the output!
constexpr bool UPDATE_SNAPSHOTS = false;

const QString CATEGORY = u"EventSub"_s;

/// JSON for the `subscription` object in `payload`
const std::map<QString, std::string_view, QCompareCaseInsensitive>
    SUBSCRIPTIONS{
        {
            "channel-moderate",
            R"({
            "id": "7297f7eb-3bf5-461f-8ae6-7cd7781ebce3",
            "status": "enabled",
            "type": "channel.moderate",
            "version": "2",
            "cost": 0,
            "condition": {
                "broadcaster_user_id": "11148817",
                "moderator_user_id": "11148817"
            },
            "transport": {
                "method": "websocket",
                "session_id": "AgoQUlB8aB2SSsavWVfcs5ljnBIGY2VsbC1j"
            },
            "created_at": "2024-02-23T21:12:33.771005262Z"
        })",
        },
        {
            "channel-ban",
            R"({
            "id": "4aa632e0-fca3-590b-e981-bbd12abdb3fe",
            "status": "enabled",
            "type": "channel.ban",
            "version": "1",
            "condition": { "broadcaster_user_id": "11148817" },
            "transport": { "method": "websocket", "session_id": "38de428e_b11f07be" },
            "created_at": "2023-05-20T12:30:55.518375571Z",
            "cost": 0
        })",
        },
        {
            "automod-message-hold",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "automod.message.hold",
            "version": "2",
            "condition": {
                "broadcaster_user_id": "489584266",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
        {
            "automod-message-update",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "automod.message.update",
            "version": "2",
            "condition": {
                "broadcaster_user_id": "489584266",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
        {
            "channel-suspicious-user-message",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "channel.suspicious_user.message",
            "version": "1",
            "condition": {
                "broadcaster_user_id": "489584266",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
        {
            "channel-suspicious-user-update",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "channel.suspicious_user.update",
            "version": "1",
            "condition": {
                "broadcaster_user_id": "489584266",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
        {
            "channel-chat-user-message-hold",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "channel.chat.user_message_hold",
            "version": "1",
            "condition": {
                "broadcaster_user_id": "11148817",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
        {
            "channel-chat-user-message-update",
            R"({
            "id": "a3122e32-6498-4847-8675-109b9b94f29c",
            "status": "enabled",
            "type": "channel.chat.user_message_update",
            "version": "1",
            "condition": {
                "broadcaster_user_id": "11148817",
                "moderator_user_id": "489584266"
            },
            "transport": {
                "method":"websocket",
                "session_id":"AgoQ59RRLw0mS6S000QtK8f54BIGY2VsbC1j"
            },
            "created_at": "2025-02-28T15:55:37.85489173Z",
            "cost": 0
        })",
        },
    };

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : highlights(this->settings, &this->accounts)
    {
    }

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }

    mock::EmptyLogging logging;
    mock::MockTwitchIrcServer twitch;
    AccountController accounts;
    HighlightController highlights;
};

std::shared_ptr<TwitchChannel> makeMockTwitchChannel(const QString &name)
{
    auto chan = std::make_shared<TwitchChannel>(name);
    return chan;
}

boost::beast::flat_buffer makePayload(std::string_view subJson,
                                      QJsonObject event)
{
    auto subscription =
        QJsonDocument::fromJson(
            QByteArray::fromRawData(subJson.data(),
                                    static_cast<qsizetype>(subJson.size())))
            .object();

    QString timestamp = "2024-05-14T12:31:47.995298776Z";
    if (event.contains("__timestamp"))
    {
        timestamp = event["__timestamp"].toString();
        event.remove("__timestamp");
    }

    QJsonObject metadata{
        {"message_id", "e8edc592-5550-4aa5-bba6-39e31a2be435"},
        {"message_type", "notification"},
        {"message_timestamp", timestamp},
        {"subscription_type", subscription["type"]},
        {"subscription_version", subscription["version"]},
    };
    QJsonObject payload{
        {"metadata", metadata},
        {"payload",
         QJsonObject{
             {"subscription", subscription},
             {"event", event},
         }},
    };

    auto bytes = QJsonDocument(payload).toJson();

    boost::beast::flat_buffer buf;
    auto inner = buf.prepare(bytes.size());
    std::memcpy(inner.data(), bytes.data(), inner.size());
    buf.commit(inner.size());
    return buf;
}

}  // namespace

class TestEventSubMessagesP : public ::testing::TestWithParam<QString>
{
public:
    void SetUp() override
    {
        auto param = TestEventSubMessagesP::GetParam();
        this->snapshot = testlib::Snapshot::read(CATEGORY, param);

        this->mockApplication = std::make_unique<MockApplication>();

        this->mainChannel = makeMockTwitchChannel("pajlada");
        this->mainChannel->setRoomId("11148817");

        this->mockApplication->twitch.mockChannels.emplace("pajlada",
                                                           this->mainChannel);
    }

    void TearDown() override
    {
        this->mainChannel.reset();
        this->mockApplication.reset();
        this->snapshot.reset();
    }

    std::shared_ptr<TwitchChannel> mainChannel;
    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<testlib::Snapshot> snapshot;
};

TEST_P(TestEventSubMessagesP, Run)
{
    QStringView subcategory(snapshot->name());
    subcategory = subcategory.sliced(0, subcategory.indexOf('/'));
    auto subscription = SUBSCRIPTIONS.find(subcategory);
    ASSERT_NE(subscription, SUBSCRIPTIONS.end()) << subcategory;

    QJsonArray input{snapshot->input()};
    if (snapshot->input().isArray())
    {
        input = snapshot->input().toArray();
    }

    std::unique_ptr<eventsub::lib::Listener> listener =
        std::make_unique<eventsub::Connection>();
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl(
        boost::asio::ssl::context::method::tls_client);
    auto sess =
        std::make_shared<eventsub::lib::Session>(ioc, ssl, std::move(listener));

    for (const auto inputRef : input)
    {
        auto inputObj = inputRef.toObject();

        // "__subscription" overrides the subscription type the message is built
        // as. By default, the subcategory (directory) name is used.
        auto eventSubscription = subscription;
        if (inputObj.contains(u"__subscription"))
        {
            eventSubscription =
                SUBSCRIPTIONS.find(inputObj[u"__subscription"].toString());
            ASSERT_NE(eventSubscription, SUBSCRIPTIONS.end());
            inputObj.remove("__subscription");
        }

        auto json = makePayload(eventSubscription->second, inputObj);

        auto ec = sess->handleMessage(json);
        ASSERT_FALSE(ec.failed())
            << ec.what() << ec.message() << ec.location().to_string();
    }

    auto messages = mainChannel->getMessageSnapshot();
    QJsonArray output;
    for (const auto &message : messages)
    {
        output.append(message->toJson());
    }

    ASSERT_TRUE(snapshot->run(output, UPDATE_SNAPSHOTS))
        << "Snapshot " << snapshot->name() << " failed. Expected JSON to be\n"
        << QJsonDocument(snapshot->output().toArray()).toJson() << "\nbut got\n"
        << QJsonDocument(output).toJson() << "\ninstead.";
}

INSTANTIATE_TEST_SUITE_P(
    EventSubMessages, TestEventSubMessagesP,
    testing::ValuesIn(testlib::Snapshot::discoverNested(CATEGORY)));

TEST(TestEventSubMessagesP, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}
