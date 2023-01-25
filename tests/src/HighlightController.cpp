#include "controllers/highlights/HighlightController.hpp"

#include "Application.hpp"
#include "BaseSettings.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/MessageBuilder.hpp"  // for MessageParseArgs
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

class MockHelix : public IHelix
{
public:
    MOCK_METHOD(void, fetchUsers,
                (QStringList userIds, QStringList userLogins,
                 ResultCallback<std::vector<HelixUser>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getUserByName,
                (QString userName, ResultCallback<HelixUser> successCallback,
                 HelixFailureCallback failureCallback),
                (override));
    MOCK_METHOD(void, getUserById,
                (QString userId, ResultCallback<HelixUser> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, fetchUsersFollows,
                (QString fromId, QString toId,
                 ResultCallback<HelixUsersFollowsResponse> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getUserFollowers,
                (QString userId,
                 ResultCallback<HelixUsersFollowsResponse> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, fetchStreams,
                (QStringList userIds, QStringList userLogins,
                 ResultCallback<std::vector<HelixStream>> successCallback,
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, getStreamById,
                (QString userId,
                 (ResultCallback<bool, HelixStream> successCallback),
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, getStreamByName,
                (QString userName,
                 (ResultCallback<bool, HelixStream> successCallback),
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, fetchGames,
                (QStringList gameIds, QStringList gameNames,
                 (ResultCallback<std::vector<HelixGame>> successCallback),
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, updateStreamTags,
                (QString broadcasterId, QStringList tags,
                 std::function<void()> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getStreamTags,
                (QString broadcasterId,
                 ResultCallback<std::vector<HelixTag>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(
        void, fetchStreamTags,
        (QString after,
         (ResultCallback<std::vector<HelixTag>, QString> successCallback),
         HelixFailureCallback failureCallback),
        (override));

    MOCK_METHOD(void, searchGames,
                (QString gameName,
                 ResultCallback<std::vector<HelixGame>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getGameById,
                (QString gameId, ResultCallback<HelixGame> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, createClip,
                (QString channelId, ResultCallback<HelixClip> successCallback,
                 std::function<void(HelixClipError)> failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, getChannel,
                (QString broadcasterId,
                 ResultCallback<HelixChannel> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, createStreamMarker,
                (QString broadcasterId, QString description,
                 ResultCallback<HelixStreamMarker> successCallback,
                 std::function<void(HelixStreamMarkerError)> failureCallback),
                (override));

    MOCK_METHOD(void, loadBlocks,
                (QString userId,
                 ResultCallback<std::vector<HelixBlock>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, blockUser,
                (QString targetUserId, std::function<void()> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, unblockUser,
                (QString targetUserId, std::function<void()> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, updateChannel,
                (QString broadcasterId, QString gameId, QString language,
                 QString title,
                 std::function<void(NetworkResult)> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, manageAutoModMessages,
                (QString userID, QString msgID, QString action,
                 std::function<void()> successCallback,
                 std::function<void(HelixAutoModMessageError)> failureCallback),
                (override));

    MOCK_METHOD(void, getCheermotes,
                (QString broadcasterId,
                 ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getEmoteSetData,
                (QString emoteSetId,
                 ResultCallback<HelixEmoteSetData> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getChannelEmotes,
                (QString broadcasterId,
                 ResultCallback<std::vector<HelixChannelEmote>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateUserChatColor,
                (QString userID, QString color,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixUpdateUserChatColorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, deleteChatMessages,
                (QString broadcasterID, QString moderatorID, QString messageID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixDeleteChatMessagesError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, addChannelModerator,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixAddChannelModeratorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, removeChannelModerator,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixRemoveChannelModeratorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, sendChatAnnouncement,
                (QString broadcasterID, QString moderatorID, QString message,
                 HelixAnnouncementColor color, ResultCallback<> successCallback,
                 (FailureCallback<HelixSendChatAnnouncementError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, addChannelVIP,
        (QString broadcasterID, QString userID,
         ResultCallback<> successCallback,
         (FailureCallback<HelixAddChannelVIPError, QString> failureCallback)),
        (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, removeChannelVIP,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixRemoveChannelVIPError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, unbanUser,
        (QString broadcasterID, QString moderatorID, QString userID,
         ResultCallback<> successCallback,
         (FailureCallback<HelixUnbanUserError, QString> failureCallback)),
        (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(  // /raid
        void, startRaid,
        (QString fromBroadcasterID, QString toBroadcasterId,
         ResultCallback<> successCallback,
         (FailureCallback<HelixStartRaidError, QString> failureCallback)),
        (override));  // /raid

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(  // /unraid
        void, cancelRaid,
        (QString broadcasterID, ResultCallback<> successCallback,
         (FailureCallback<HelixCancelRaidError, QString> failureCallback)),
        (override));  // /unraid

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateEmoteMode,
                (QString broadcasterID, QString moderatorID, bool emoteMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateFollowerMode,
                (QString broadcasterID, QString moderatorID,
                 boost::optional<int> followerModeDuration,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateNonModeratorChatDelay,
                (QString broadcasterID, QString moderatorID,
                 boost::optional<int> nonModeratorChatDelayDuration,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateSlowMode,
                (QString broadcasterID, QString moderatorID,
                 boost::optional<int> slowModeWaitTime,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateSubscriberMode,
                (QString broadcasterID, QString moderatorID,
                 bool subscriberMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateUniqueChatMode,
                (QString broadcasterID, QString moderatorID,
                 bool uniqueChatMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));
    // update chat settings

    // /timeout, /ban
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, banUser,
                (QString broadcasterID, QString moderatorID, QString userID,
                 boost::optional<int> duration, QString reason,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixBanUserError, QString> failureCallback)),
                (override));  // /timeout, /ban

    // /w
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, sendWhisper,
                (QString fromUserID, QString toUserID, QString message,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixWhisperError, QString> failureCallback)),
                (override));  // /w

    // getChatters
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, getChatters,
        (QString broadcasterID, QString moderatorID, int maxChattersToFetch,
         ResultCallback<HelixChatters> successCallback,
         (FailureCallback<HelixGetChattersError, QString> failureCallback)),
        (override));  // getChatters

    // /vips
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, getChannelVIPs,
        (QString broadcasterID,
         ResultCallback<std::vector<HelixVip>> successCallback,
         (FailureCallback<HelixListVIPsError, QString> failureCallback)),
        (override));  // /vips

    // /commercial
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, startCommercial,
        (QString broadcasterID, int length,
         ResultCallback<HelixStartCommercialResponse> successCallback,
         (FailureCallback<HelixStartCommercialError, QString> failureCallback)),
        (override));  // /commercial

    // /mods
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(
        void, getModerators,
        (QString broadcasterID, int maxModeratorsToFetch,
         ResultCallback<std::vector<HelixModerator>> successCallback,
         (FailureCallback<HelixGetModeratorsError, QString> failureCallback)),
        (override));  // /mods

    MOCK_METHOD(void, update, (QString clientId, QString oauthToken),
                (override));

protected:
    // The extra parenthesis around the failure callback is because its type contains a comma
    MOCK_METHOD(void, updateChatSettings,
                (QString broadcasterID, QString moderatorID, QJsonObject json,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));
};

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
        {
            // Write default settings to the mock settings json file
            QDir().mkpath("/tmp/c2-tests");
            QFile settingsFile("/tmp/c2-tests/settings.json");
            assert(settingsFile.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream out(&settingsFile);
            out << DEFAULT_SETTINGS;
        }

        this->mockHelix = new MockHelix;

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
        QDir().rmdir("/tmp/c2-tests");
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

    MockHelix *mockHelix;
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
