#include "controllers/highlights/HighlightController.hpp"

#include "Application.hpp"
#include "BaseSettings.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/MessageBuilder.hpp"  // for MessageParseArgs
#include "mocks/Helix.hpp"
#include "mocks/UserData.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchBadge.hpp"  // for Badge
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <boost/optional/optional_io.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : IApplication
{
public:
    Theme *getThemes() override
    {
        return nullptr;
    }
    Fonts *getFonts() override
    {
        return nullptr;
    }
    IEmotes *getEmotes() override
    {
        return nullptr;
    }
    AccountController *getAccounts() override
    {
        return &this->accounts;
    }
    HotkeyController *getHotkeys() override
    {
        return nullptr;
    }
    WindowManager *getWindows() override
    {
        return nullptr;
    }
    Toasts *getToasts() override
    {
        return nullptr;
    }
    CommandController *getCommands() override
    {
        return nullptr;
    }
    NotificationController *getNotifications() override
    {
        return nullptr;
    }
    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }
    TwitchIrcServer *getTwitch() override
    {
        return nullptr;
    }
    ChatterinoBadges *getChatterinoBadges() override
    {
        return nullptr;
    }
    FfzBadges *getFfzBadges() override
    {
        return nullptr;
    }
    IUserDataController *getUserData() override
    {
        return &this->userData;
    }

    AccountController accounts;
    HighlightController highlights;
    mock::UserDataController userData;
    // TODO: Figure this out
};

}  // namespace

static QString DEFAULT_SETTINGS = R"!(
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

struct TestCase {
    // TODO: create one of these from a raw irc message? hmm xD
    struct {
        MessageParseArgs args;
        std::vector<Badge> badges;
        QString senderName;
        QString originalMessage;
        MessageFlags flags;
    } input;

    struct {
        bool state;
        HighlightResult result;
    } expected;
};

class HighlightControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Write default settings to the mock settings json file
        ASSERT_TRUE(QDir().mkpath("/tmp/c2-tests"));

        QFile settingsFile("/tmp/c2-tests/settings.json");
        ASSERT_TRUE(settingsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        ASSERT_GT(settingsFile.write(DEFAULT_SETTINGS.toUtf8()), 0);
        ASSERT_TRUE(settingsFile.flush());
        settingsFile.close();

        this->mockHelix = new mock::Helix;

        initializeHelix(this->mockHelix);

        EXPECT_CALL(*this->mockHelix, loadBlocks).Times(Exactly(1));
        EXPECT_CALL(*this->mockHelix, update).Times(Exactly(1));

        this->mockApplication = std::make_unique<MockApplication>();
        this->settings = std::make_unique<Settings>("/tmp/c2-tests");
        this->paths = std::make_unique<Paths>();

        this->controller = std::make_unique<HighlightController>();

        this->mockApplication->accounts.initialize(*this->settings,
                                                   *this->paths);
        this->controller->initialize(*this->settings, *this->paths);
    }

    void TearDown() override
    {
        ASSERT_TRUE(QDir("/tmp/c2-tests").removeRecursively());
        this->mockApplication.reset();
        this->settings.reset();
        this->paths.reset();

        this->controller.reset();

        delete this->mockHelix;
    }

    std::unique_ptr<MockApplication> mockApplication;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<Paths> paths;

    std::unique_ptr<HighlightController> controller;

    mock::Helix *mockHelix;
};

TEST_F(HighlightControllerTest, A)
{
    auto currentUser =
        this->mockApplication->getAccounts()->twitch.getCurrent();
    std::vector<TestCase> tests{
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "pajlada",           // sender name
                "hello!",            // original message
            },
            {
                // expected
                true,  // state
                {
                    false,                                  // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7fffffff"),  // color
                    false,
                },
            },
        },
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
            {
                // input
                MessageParseArgs{},  // no special args
                {
                    {
                        "founder",
                        "0",
                    },  // founder badge
                },
                "pajlada22",  // sender name
                "hello!",     // original message
            },
            {
                // expected
                true,  // state
                {
                    true,                                   // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7fe8b7eb"),  // color
                    false,                                  //showInMentions
                },
            },
        },
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {
                    {
                        "founder",
                        "0",
                    },  // founder badge
                },
                "pajlada",  // sender name
                "hello!",   // original message
            },
            {
                // expected
                true,  // state
                {
                    true,                                   // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7fffffff"),  // color
                    false,                                  //showInMentions
                },
            },
        },
        {
            // Badge highlight with showInMentions only
            {
                // input
                MessageParseArgs{},  // no special args
                {
                    {
                        "vip",
                        "0",
                    },
                },
                "badge",                  // sender name
                "show in mentions only",  // original message
            },
            {
                // expected
                true,  // state
                {
                    false,                                  // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7fe8b7ec"),  // color
                    true,                                   // showInMentions
                },
            },
        },
        {
            // User mention with showInMentions
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "gempir",            // sender name
                "a",                 // original message
            },
            {
                // expected
                true,  // state
                {
                    true,                                   // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7ff19900"),  // color
                    true,                                   // showInMentions
                },
            },
        },
        {
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "a",                 // sender name
                "!testmanxd",        // original message
            },
            {
                // expected
                true,  // state
                {
                    true,                                   // alert
                    true,                                   // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#7f7f3f49"),  // color
                    true,                                   // showInMentions
                },
            },
        },
        {
            // TEST CASE: Message phrase from sender should be ignored (so showInMentions false), but since it's a user highlight, it should set a color
            {
                // input
                MessageParseArgs{},  // no special args
                {},                  // no badges
                "testaccount_420",   // sender name
                "!testmanxd",        // original message
            },
            {
                // expected
                true,  // state
                {
                    false,                                  // alert
                    false,                                  // playsound
                    boost::none,                            // custom sound url
                    std::make_shared<QColor>("#6fffffff"),  // color
                    false,
                },
            },
        },
        {
            // TEST CASE: Whispers that do not hit a highlight phrase should not be added to /mentions
            {
                // input
                .args =
                    MessageParseArgs{
                        .isReceivedWhisper = true,
                    },
                .senderName = "forsen",
                .originalMessage = "Hello NymN!",
            },
            {
                // expected
                .state = true,  // state
                .result =
                    {
                        false,        // alert
                        false,        // playsound
                        boost::none,  // custom sound url
                        std::make_shared<QColor>(
                            HighlightPhrase::
                                FALLBACK_HIGHLIGHT_COLOR),  // color
                        false,                              // showInMentions
                    },
            },
        },
        {
            // TEST CASE: Whispers that do hit a highlight phrase should be added to /mentions
            {
                // input
                .args =
                    MessageParseArgs{
                        .isReceivedWhisper = true,
                    },
                .senderName = "forsen",
                .originalMessage = "!testmanxd",
            },
            {
                // expected
                .state = true,  // state
                .result =
                    {
                        true,         // alert
                        true,         // playsound
                        boost::none,  // custom sound url
                        std::make_shared<QColor>("#7f7f3f49"),  // color
                        true,  // showInMentions
                    },
            },
        },
    };

    for (const auto &[input, expected] : tests)
    {
        auto [isMatch, matchResult] =
            this->controller->check(input.args, input.badges, input.senderName,
                                    input.originalMessage, input.flags);

        EXPECT_EQ(isMatch, expected.state)
            << qUtf8Printable(input.senderName) << ": "
            << qUtf8Printable(input.originalMessage);
        EXPECT_EQ(matchResult, expected.result)
            << qUtf8Printable(input.senderName) << ": "
            << qUtf8Printable(input.originalMessage);
    }
}
