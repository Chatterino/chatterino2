// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/HighlightController.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"  // for MessageParseArgs
#include "mocks/BaseApplication.hpp"
#include "mocks/Helix.hpp"
#include "mocks/UserData.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchBadge.hpp"  // for Badge
#include "Test.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

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
                "color": "#7fffffff"
            },
            {
                "pattern": "testaccount_420",
                "showInMentions": false,
                "alert": false,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#6fffffff"
            },
            {
                "pattern": "gempir",
                "showInMentions": true,
                "alert": true,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#7ff19900"
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
                "color": "#7f7f3f49"
            }
        ],
        "badges": [
            {
                "name": "broadcaster",
                "displayName": "Broadcaster",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f427f00"
            },
            {
                "name": "subscriber",
                "displayName": "Subscriber",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f7f3f49"
            },
            {
                "name": "founder",
                "displayName": "Founder",
                "alert": true,
                "sound": false,
                "soundUrl": "",
                "color": "#7fe8b7eb"
            },
            {
                "name": "vip",
                "displayName": "VIP",
                "showInMentions": true,
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7fe8b7ec"
            }
        ],
        "subHighlightColor": "#64ffd641"
    }
})!";

static QString SETTINGS_DEFAULT_V2 = R"!(
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
                "color": "#7fffffff"
            },
            {
                "pattern": "testaccount_420",
                "showInMentions": false,
                "alert": false,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#6fffffff"
            },
            {
                "pattern": "gempir",
                "showInMentions": true,
                "alert": true,
                "sound": false,
                "regex": false,
                "case": false,
                "soundUrl": "",
                "color": "#7ff19900"
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
                "color": "#7f7f3f49"
            }
        ],
        "badges": [
            {
                "name": "broadcaster",
                "displayName": "Broadcaster",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f427f00"
            },
            {
                "name": "subscriber",
                "displayName": "Subscriber",
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7f7f3f49"
            },
            {
                "name": "founder",
                "displayName": "Founder",
                "alert": true,
                "sound": false,
                "soundUrl": "",
                "color": "#7fe8b7eb"
            },
            {
                "name": "vip",
                "displayName": "VIP",
                "showInMentions": true,
                "alert": false,
                "sound": false,
                "soundUrl": "",
                "color": "#7fe8b7ec"
            }
        ],
        "subHighlightColor": "#64ffd641"
    }
})!";

static QString SETTINGS_ANON_EMPTY = R"!(
{
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
            auto [isMatch, matchResult] =
                this->mockApplication->getHighlights()->check(
                    input.args, input.badges, input.senderName,
                    input.originalMessage, input.flags, input.runContext);

            ASSERT_EQ(isMatch, expected.state)
                << '[' << input.testName << "] " << input.senderName << ": "
                << input.originalMessage;
            ASSERT_EQ(matchResult, expected.result)
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
                            .playSound = false,
                            .customSoundUrl = std::nullopt,
                            .color = std::make_shared<QColor>("#7fffffff"),
                            .showInMentions = false,
                        },
                },
        },
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
            .input =
                {
                    // input
                    .args = MessageParseArgs{},  // no special args
                    .badges =
                        {
                            {
                                "founder",
                                "0",
                            },  // founder badge
                        },
                    .senderName = "pajlada22",    // sender name
                    .originalMessage = "hello!",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = true,                   // alert
                            .playSound = false,              // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7fe8b7eb"),  // color
                            .showInMentions = false,  //showInMentions
                        },
                },
        },
        {
            .input =
                {
                    // input
                    .args = MessageParseArgs{},  // no special args
                    .badges =
                        {
                            {
                                "founder",
                                "0",
                            },  // founder badge
                        },
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
                            .alert = true,                   // alert
                            .playSound = false,              // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7fffffff"),  // color
                            .showInMentions = false,  //showInMentions
                        },
                },
        },
        {
            // Badge highlight with showInMentions only
            .input =
                {
                    // input
                    .args = MessageParseArgs{},  // no special args
                    .badges =
                        {
                            {
                                "vip",
                                "0",
                            },
                        },
                    .senderName = "badge",  // sender name
                    .originalMessage =
                        "show in mentions only",  // original message
                    .runContext = ctx,
                },
            .expected =
                {
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = false,                  // alert
                            .playSound = false,              // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7fe8b7ec"),  // color
                            .showInMentions = true,  // showInMentions
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
                    // expected
                    .state = true,  // state
                    .result =
                        {
                            .alert = true,                   // alert
                            .playSound = false,              // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7ff19900"),  // color
                            .showInMentions = true,  // showInMentions
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
                            .alert = true,                   // alert
                            .playSound = true,               // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7f7f3f49"),  // color
                            .showInMentions = true,  // showInMentions
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
                    .state = true,  // state
                    .result =
                        {
                            .alert = false,                  // alert
                            .playSound = false,              // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#6fffffff"),  // color
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
                            .playSound = false,
                            .customSoundUrl = std::nullopt,
                            .color = std::make_shared<QColor>(
                                HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR),
                            .showInMentions = false,
                        },
                },
        },
        {
            // TEST CASE: Whispers that do hit a highlight phrase should be added to /mentions
            .input =
                {
                    // input
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
                            .alert = true,                   // alert
                            .playSound = true,               // playsound
                            .customSoundUrl = std::nullopt,  // custom sound url
                            .color =
                                std::make_shared<QColor>("#7f7f3f49"),  // color
                            .showInMentions = true,  // showInMentions
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

#if 0
TEST_F(HighlightControllerTest, Pajlada)
{
    configure(SETTINGS_DEFAULT, false);

    SharedHighlight2 highlight{
        .id = "test",
    };

    std::vector<TestCase> tests{
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "pajlada2",          // sender name
                "hello!",            // original message
            },
            {
                // expected
                false,                           // state
                HighlightResult::emptyResult(),  // result
            },
        },
        {
            // anonymous default username
            {
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "pajlada2",          // sender name
                "justinfan64537",    // original message
            },
            {
                // expected
                false,                           // state
                HighlightResult::emptyResult(),  // result
            },
        },
    };

    this->runTests(tests);
}
#endif
