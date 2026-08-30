// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/HighlightController.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "controllers/highlights/types/All.hpp"
#include "controllers/highlights/types/WhispersHighlight.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"  // for MessageParseArgs
#include "mocks/BaseApplication.hpp"
#include "mocks/Helix.hpp"
#include "mocks/UserData.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"  // for Badge
#include "Test.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include <variant>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication(const QString &settingsBody)
        : mock::BaseApplication(settingsBody, /*runMigrations*/ true)
        , highlights(this->settings, &this->accounts)
    {
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }

    IUserDataController *getUserData() override
    {
        return &this->userData;
    }

    AccountController accounts;
    HighlightController highlights;
    mock::UserDataController userData;
};

}  // namespace

static QString SETTINGS_DEFAULT = R"!(
{
    "accounts": {
        "uid117166826": {
            "username": "testaccount_420",
            "userID": "117166826",
            "clientID": "abc",
            "oauthToken": "def"
        },
        "current": "testaccount_420"
    },
    "highlighting": {
        "selfHighlight": {
            "enableSound": true
        },
        "blacklist": [
            {
                "pattern": "zenix",
                "regex": false
            }
        ],
        "users": [
            {
                "pattern": "pajlada",
                "showInMentions": false,
                "alert": false,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#30000001"
            },
            {
                "pattern": "testaccount_420",
                "showInMentions": false,
                "alert": false,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#30000002"
            },
            {
                "pattern": "gempir",
                "showInMentions": true,
                "alert": true,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#30000003"
            }
        ],
        "alwaysPlaySound": true,
        "highlights": [
            {
                "pattern": "!testmanxd",
                "showInMentions": true,
                "alert": true,
                "sound": true,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#10000001"
            }
        ],
        "badges": [
            {
                "name": "broadcaster",
                "displayName": "Broadcaster",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#20000001"
            },
            {
                "name": "subscriber",
                "displayName": "Subscriber",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#20000002"
            },
            {
                "name": "founder",
                "displayName": "Founder",
                "alert": true,
                "sound": false,
                "soundUrl": "",
                "color": "#20000003"
            },
            {
                "name": "vip",
                "displayName": "VIP",
                "showInMentions": true,
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#20000004"
            }
        ],
        "subHighlightColor": "#64ffd641"
    }
})!";

static QString SETTINGS_ANON_EMPTY = R"!(
{
})!";

static QString SETTINGS_MIGRATED_EMPTY = R"!(
{
    "misc": {
        "settingsVersion": 1
    }
})!";

struct TestCase {
    // TODO: create one of these from a raw irc message? hmm xD
    struct {
        QString testName = "Undefined Test Name";
        MessageParseArgs args;
        std::vector<TwitchBadge> badges;
        QString senderName;
        QString originalMessage;
        MessageFlags flags;
        filters::RunContext runContext;
    } input;

    struct {
        bool state;
        HighlightResult result;
    } expected;
};

class HighlightControllerTest : public ::testing::Test
{
protected:
    void configure(const QString &settings, bool isAnon)
    {
        // Write default settings to the mock settings json file
        this->mockApplication = std::make_unique<MockApplication>(settings);

        this->mockHelix = new mock::Helix;

        initializeHelix(this->mockHelix);

        EXPECT_CALL(*this->mockHelix, loadBlocks).Times(Exactly(1));
        EXPECT_CALL(*this->mockHelix, update).Times(Exactly(isAnon ? 0 : 1));

        this->mockApplication->accounts.load();
    }

    void runTests(const std::vector<TestCase> &tests)
    {
        for (const auto &[input, expected] : tests)
        {
            auto [isMatch,
                  matchResult] = this->mockApplication->getHighlights()->check({
                .args = input.args,
                .twitchBadges = input.badges,
                .senderName = input.senderName,
                .originalMessage = input.originalMessage,
                .messageFlags = input.flags,
                .self = input.senderName == this->mockApplication->getAccounts()
                                                ->twitch.getCurrent()
                                                ->getUserName(),
                .runContext = input.runContext,
            });

            EXPECT_EQ(isMatch, expected.state)
                << '[' << input.testName << "] " << input.senderName << ": "
                << input.originalMessage;
            EXPECT_EQ(matchResult, expected.result)
                << '[' << input.testName << "] " << input.senderName << ": "
                << input.originalMessage;
        }
    }

    void TearDown() override
    {
        this->mockApplication.reset();

        delete this->mockHelix;
    }

    std::unique_ptr<MockApplication> mockApplication;

    mock::Helix *mockHelix;
};

namespace chatterino {

TEST_F(HighlightControllerTest, LoggedInAndConfigured)
{
    configure(SETTINGS_DEFAULT, false);

    Message message;
    message.displayName = "icelys";
    message.usernameColor = QColor(0xff0000);
    message.messageText = "hey there :) 2038-01-19 123 456";
    message.channelName = "forsen";
    message.twitchBadges = {
        TwitchBadge("moderator", ""),
        TwitchBadge("staff", ""),
    };
    message.externalBadges = {"frankerfacez:bot"};
    filters::RunContext ctx{
        .message = message,
        .channel = nullptr,
    };

    std::vector<TestCase> tests{
        {
            .input =
                {
                    .testName = "[User Highlight] Match 001",
                    .args = MessageParseArgs{},   // no special args
                    .badges = {},                 // no badges
                    .senderName = "pajlada",      // sender name
                    .originalMessage = "hello!",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = false,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>("#30000001"),
                            .showInMentions = false,
                        },
                },
        },
        {
            .input =
                {
                    .testName = "No Match 001",
                    .args = MessageParseArgs{},   // no special args
                    .badges = {},                 // no badges
                    .senderName = "pajlada2",     // sender name
                    .originalMessage = "hello!",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = false,                            // state
                    .result = HighlightResult::emptyResult(),  // result
                },
        },
        {
            .input =
                {
                    .testName = "[Badge Highlight] Match Founder Badge 001",
                    .args = MessageParseArgs{},
                    .badges =
                        {
                            {
                                "founder",
                                "0",
                            },  // founder badge
                        },
                    .senderName = "pajlada22",
                    .originalMessage = "hello!",
                    .runContext = ctx,
                },
            .expected =
                {
                    .state = true,
                    .result =
                        {
                            .alert = true,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>("#20000003"),
                            .showInMentions = false,
                        },
                },
        },
        {
            .input =
                {
                    .testName = "[User Highlight + Badge Highlight] Match 001",
                    .args = MessageParseArgs{},
                    .badges =
                        {
                            {
                                "founder",
                                "0",
                            },  // founder badge
                        },
                    .senderName = "pajlada",
                    .originalMessage = "hello!",
                    .runContext = ctx,
                },
            .expected =
                {
                    .state = true,
                    .result =
                        {
                            .alert = true,
                            .sound = QUrl{},
                            // Color comes from User Highlight - user highlights, by default, have a higher priority than badge highlights
                            .color = std::make_shared<QColor>("#30000001"),
                            .showInMentions = false,
                        },
                },
        },
        {
            // Badge highlight with showInMentions only
            .input =
                {
                    .testName = "[Badge Highlight] alert disabled, show in "
                                "mentions enabled",
                    .args = MessageParseArgs{},  // no special args
                    .badges =
                        {
                            {
                                "vip",
                                "0",
                            },
                        },
                    .senderName = "badge",
                    .originalMessage = "show in mentions only",
                    .runContext = ctx,
                },
            .expected =
                {
                    .state = true,
                    .result =
                        {
                            .alert = false,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>("#20000004"),
                            .showInMentions = true,
                        },
                },
        },
        {
            // User mention with showInMentions
            .input =
                {
                    // input
                    .args = MessageParseArgs{},  // no special args
                    .badges = {},                // no badges
                    .senderName = "gempir",      // sender name
                    .originalMessage = "a",      // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    .state = true,
                    .result =
                        {
                            .alert = true,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>("#30000003"),
                            .showInMentions = true,
                        },
                },
        },
        {
            .input =
                {
                    .testName = "Test A",
                    .args = MessageParseArgs{},       // no special args
                    .badges = {},                     // no badges
                    .senderName = "a",                // sender name
                    .originalMessage = "!testmanxd",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = true,
                            .sound = QUrl{"qrc:/sounds/ping2.wav"},
                            // !testmanxd message highlight color
                            .color = std::make_shared<QColor>("#10000001"),
                            .showInMentions = true,
                        },
                },
        },
        {
            // TEST CASE: Message phrase from sender should be ignored (so showInMentions false), but since it's a user highlight, it should set a color
            .input =
                {
                    .testName = "MessageHighlight from sender should be "
                                "ignored, but UserHighlight should not",
                    .args = MessageParseArgs{},       // no special args
                    .badges = {},                     // no badges
                    .senderName = "testaccount_420",  // sender name
                    .originalMessage = "!testmanxd",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,
                    .result =
                        {
                            .alert = false,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>("#30000002"),
                            .showInMentions = false,
                        },
                },
        },
        {
            // TEST CASE: Whispers that do not hit a highlight phrase should not be added to /mentions
            .input =
                {
                    // input
                    .args =
                        MessageParseArgs{
                            .isReceivedWhisper = true,
                        },
                    .senderName = "forsen",
                    .originalMessage = "Hello NymN!",
                    .runContext = ctx,
                },
            .expected =
                {
                    .state = true,
                    .result =
                        {
                            .alert = false,
                            .sound = QUrl{},
                            .color = std::make_shared<QColor>(
                                highlights::WhispersHighlight::
                                    BACKGROUND_COLOR_DEFAULT),
                            .showInMentions = false,
                        },
                },
        },
        {
            // TEST CASE: Whispers that do hit a highlight phrase should be added to /mentions
            .input =
                {
                    .testName = "[Whispers Highlight] Match 001",
                    .args =
                        MessageParseArgs{
                            .isReceivedWhisper = true,
                        },
                    .senderName = "forsen",
                    .originalMessage = "!testmanxd",
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = true,
                            .sound = QUrl{"qrc:/sounds/ping2.wav"},
                            .color = std::make_shared<QColor>(
                                highlights::WhispersHighlight::
                                    BACKGROUND_COLOR_DEFAULT),
                            .showInMentions = true,
                        },
                },
        },
    };

    this->runTests(tests);
}

TEST_F(HighlightControllerTest, AnonEmpty)
{
    configure(SETTINGS_ANON_EMPTY, true);

    Message message;
    message.loginName = "pajlada2";
    message.displayName = "pajlada2";
    message.usernameColor = QColor(0xff0000);
    message.messageText = "hello!";
    message.channelName = "forsen";
    message.twitchBadges = {};
    message.externalBadges = {"frankerfacez:bot"};
    filters::RunContext ctx{
        .message = message,
        .channel = nullptr,
    };

    std::vector<TestCase> tests{
        {
            .input =
                {
                    // input
                    .args = MessageParseArgs{},   // no special args
                    .badges = {},                 // no badges
                    .senderName = "pajlada2",     // sender name
                    .originalMessage = "hello!",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = false,                            // state
                    .result = HighlightResult::emptyResult(),  // result
                },
        },
        {
            // anonymous default username
            .input =
                {
                    .args = MessageParseArgs{},           // no special args
                    .badges = {},                         // no badges
                    .senderName = "pajlada2",             // sender name
                    .originalMessage = "justinfan64537",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = false,                            // state
                    .result = HighlightResult::emptyResult(),  // result
                },
        },
    };

    this->runTests(tests);
}

TEST_F(HighlightControllerTest, BillTinHighlights)
{
    configure(SETTINGS_MIGRATED_EMPTY, true);

    const auto all = std::variant_size_v<highlights::AllHighlights>;
    const auto billTin = HighlightController::billTinHighlights().size();

    const auto expectedSize = all  //
                              - 1  // MessageHighlight
                              - 1  // UserHighlight
                              - 1  // BadgeHighlight
                              - 1  // FilterHighlight
        ;

    ASSERT_EQ(expectedSize, billTin)
        << "HighlightController::billTinHighlights must include all bill tin "
           "highlights";

    ASSERT_EQ(getSettings()->sharedHighlights.raw().size(), 0)
        << "Settings should contain no highlights to start since this is a "
           "clean config that has skipped migrations";

    auto missing = HighlightController::missingBillTinHighlights();

    ASSERT_EQ(missing.size(), billTin)
        << "List of missing bill tin highlights should match the list of bill "
           "tin highlights";

    HighlightController::recreateMissingBillTinHighlights(missing);

    ASSERT_EQ(getSettings()->sharedHighlights.raw().size(), billTin)
        << "Number of highlights settings should match the list of bill tin "
           "highlights after recreate has been called";
}

}  // namespace chatterino
