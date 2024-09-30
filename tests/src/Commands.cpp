#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Emotes.hpp"
#include "mocks/Helix.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

#include <QStringBuilder>

using namespace chatterino;

using ::testing::_;
using ::testing::StrictMock;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : commands(this->paths_)
    {
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    CommandController *getCommands() override
    {
        return &this->commands;
    }

    IEmotes *getEmotes() override
    {
        return &this->emotes;
    }

    ILogging *getChatLogger() override
    {
        return &this->chatLogger;
    }

    mock::EmptyLogging chatLogger;
    AccountController accounts;
    CommandController commands;
    mock::MockTwitchIrcServer twitch;
    mock::Emotes emotes;
};

}  // namespace

namespace chatterino {

TEST(Commands, parseBanActions)
{
    MockApplication app;

    std::shared_ptr<TwitchChannel> channel =
        std::make_shared<TwitchChannel>("forsen");

    CommandContext ctx{};

    QString command("/ban");
    QString usage("usage string");
    bool withDuration = false;
    bool withReason = true;

    struct Test {
        CommandContext inputContext;

        std::vector<commands::PerformChannelAction> expectedActions;
        QString expectedError;
    };

    std::vector<Test> tests{
        {
            // Normal ban with an added reason, with the user maybe trying to use the --channel parameter at the end, but it gets eaten by the reason
            .inputContext =
                {
                    .words = {"/ban", "forsen", "the", "ban", "reason",
                              "--channel", "xD"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the ban reason --channel xD",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
        {
            // Normal ban with an added reason
            .inputContext =
                {
                    .words = {"/ban", "forsen", "the", "ban", "reason"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the ban reason",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
        {
            // Normal ban without an added reason
            .inputContext =
                {
                    .words = {"/ban", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
        {
            // User forgot to specify who to ban
            .inputContext =
                {
                    .words = {"/ban"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User tried to use /ban outside of a channel context (shouldn't really be able to happen)
            .inputContext =
                {
                    .words = {"/ban"},
                },
            .expectedActions = {},
            .expectedError =
                "A " % command %
                " action must be performed with a channel as a context",
        },
        {
            // User tried to use /ban without a target, but with a --channel specified
            .inputContext =
                {
                    .words = {"/ban", "--channel", "pajlada"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User overriding the ban to be done in the pajlada channel
            .inputContext =
                {
                    .words = {"/ban", "--channel", "pajlada", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the ban to be done in the pajlada channel and in the channel with the id 11148817
            .inputContext =
                {
                    .words = {"/ban", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 0,
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the ban to be done in the pajlada channel and in the channel with the id 11148817, with a reason specified
            .inputContext =
                {
                    .words = {"/ban", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen", "the", "ban", "reason"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the ban reason",
                        .duration = 0,
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the ban reason",
                        .duration = 0,
                    },
                },
            .expectedError = "",
        },
    };

    for (const auto &test : tests)
    {
        auto oActions = commands::parseChannelAction(
            test.inputContext, command, usage, withDuration, withReason);

        if (!test.expectedActions.empty())
        {
            ASSERT_TRUE(oActions.has_value()) << oActions.error();
            auto actions = *oActions;
            ASSERT_EQ(actions.size(), test.expectedActions.size());
            ASSERT_EQ(actions, test.expectedActions);
        }
        else
        {
            ASSERT_FALSE(oActions.has_value());
        }

        if (!test.expectedError.isEmpty())
        {
            ASSERT_FALSE(oActions.has_value());
            ASSERT_EQ(oActions.error(), test.expectedError);
        }
    }
}

TEST(Commands, parseTimeoutActions)
{
    MockApplication app;

    std::shared_ptr<TwitchChannel> channel =
        std::make_shared<TwitchChannel>("forsen");

    CommandContext ctx{};

    QString command("/timeout");
    QString usage("usage string");
    bool withDuration = true;
    bool withReason = true;

    struct Test {
        CommandContext inputContext;

        std::vector<commands::PerformChannelAction> expectedActions;
        QString expectedError;
    };

    std::vector<Test> tests{
        {
            // Normal timeout without an added reason, with the default duration
            .inputContext =
                {
                    .words = {"/timeout", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 10 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // Normal timeout without an added reason, with a custom duration
            .inputContext =
                {
                    .words = {"/timeout", "forsen", "5m"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 5 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // Normal timeout without an added reason, with a custom duration, with an added reason
            .inputContext =
                {
                    .words = {"/timeout", "forsen", "5m", "the", "timeout",
                              "reason"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the timeout reason",
                        .duration = 5 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // Normal timeout without an added reason, with an added reason, but user forgot to specify a timeout duration so it fails
            .inputContext =
                {
                    .words = {"/timeout", "forsen", "the", "timeout", "reason"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Invalid duration - " % usage,
        },
        {
            // User forgot to specify who to timeout
            .inputContext =
                {
                    .words = {"/timeout"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User tried to use /timeout outside of a channel context (shouldn't really be able to happen)
            .inputContext =
                {
                    .words = {"/timeout"},
                },
            .expectedActions = {},
            .expectedError =
                "A " % command %
                " action must be performed with a channel as a context",
        },
        {
            // User tried to use /timeout without a target, but with a --channel specified
            .inputContext =
                {
                    .words = {"/timeout", "--channel", "pajlada"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User overriding the timeout to be done in the pajlada channel
            .inputContext =
                {
                    .words = {"/timeout", "--channel", "pajlada", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 10 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the timeout to be done in the pajlada channel and in the channel with the id 11148817
            .inputContext =
                {
                    .words = {"/timeout", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 10 * 60,
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 10 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the timeout to be done in the pajlada channel and in the channel with the id 11148817, with a custom duration
            .inputContext =
                {
                    .words = {"/timeout", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen", "5m"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 5 * 60,
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "",
                        .duration = 5 * 60,
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the timeout to be done in the pajlada channel and in the channel with the id 11148817, with a reason specified
            .inputContext =
                {
                    .words = {"/timeout", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen", "10m", "the", "timeout",
                              "reason"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the timeout reason",
                        .duration = 10 * 60,
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                        .reason = "the timeout reason",
                        .duration = 10 * 60,
                    },
                },
            .expectedError = "",
        },
    };

    for (const auto &test : tests)
    {
        auto oActions = commands::parseChannelAction(
            test.inputContext, command, usage, withDuration, withReason);

        if (!test.expectedActions.empty())
        {
            ASSERT_TRUE(oActions.has_value()) << oActions.error();
            auto actions = *oActions;
            ASSERT_EQ(actions.size(), test.expectedActions.size());
            ASSERT_EQ(actions, test.expectedActions);
        }
        else
        {
            ASSERT_FALSE(oActions.has_value());
        }

        if (!test.expectedError.isEmpty())
        {
            ASSERT_FALSE(oActions.has_value());
            ASSERT_EQ(oActions.error(), test.expectedError);
        }
    }
}

TEST(Commands, parseUnbanActions)
{
    MockApplication app;

    std::shared_ptr<TwitchChannel> channel =
        std::make_shared<TwitchChannel>("forsen");

    CommandContext ctx{};

    QString command("/unban");
    QString usage("usage string");
    bool withDuration = false;
    bool withReason = false;

    struct Test {
        CommandContext inputContext;

        std::vector<commands::PerformChannelAction> expectedActions;
        QString expectedError;
    };

    std::vector<Test> tests{
        {
            // Normal unban
            .inputContext =
                {
                    .words = {"/unban", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                    },
                },
            .expectedError = "",
        },
        {
            // Normal unban but user input some random junk after the target
            .inputContext =
                {
                    .words = {"/unban", "forsen", "foo", "bar", "baz"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "forsen",
                                .displayName = "forsen",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                    },
                },
            .expectedError = "",
        },
        {
            // User forgot to specify who to unban
            .inputContext =
                {
                    .words = {"/unban"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User tried to use /unban outside of a channel context (shouldn't really be able to happen)
            .inputContext =
                {
                    .words = {"/unban"},
                },
            .expectedActions = {},
            .expectedError =
                "A " % command %
                " action must be performed with a channel as a context",
        },
        {
            // User tried to use /unban without a target, but with a --channel specified
            .inputContext =
                {
                    .words = {"/unban", "--channel", "pajlada"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions = {},
            .expectedError = "Missing target - " % usage,
        },
        {
            // User overriding the unban to be done in the pajlada channel
            .inputContext =
                {
                    .words = {"/unban", "--channel", "pajlada", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                    },
                },
            .expectedError = "",
        },
        {
            // User overriding the unban to be done in the pajlada channel and in the channel with the id 11148817
            .inputContext =
                {
                    .words = {"/unban", "--channel", "pajlada", "--channel",
                              "id:11148817", "forsen"},
                    .channel = channel,
                    .twitchChannel = channel.get(),
                },
            .expectedActions =
                {
                    {
                        .channel =
                            {
                                .login = "pajlada",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                    },
                    {
                        .channel =
                            {
                                .id = "11148817",
                            },
                        .target =
                            {
                                .login = "forsen",
                            },
                    },
                },
            .expectedError = "",
        },
    };

    for (const auto &test : tests)
    {
        auto oActions = commands::parseChannelAction(
            test.inputContext, command, usage, withDuration, withReason);

        if (!test.expectedActions.empty())
        {
            ASSERT_TRUE(oActions.has_value()) << oActions.error();
            auto actions = *oActions;
            ASSERT_EQ(actions.size(), test.expectedActions.size());
            ASSERT_EQ(actions, test.expectedActions);
        }
        else
        {
            ASSERT_FALSE(oActions.has_value());
        }

        if (!test.expectedError.isEmpty())
        {
            ASSERT_FALSE(oActions.has_value());
            ASSERT_EQ(oActions.error(), test.expectedError);
        }
    }
}

TEST(Commands, E2E)
{
    ::testing::InSequence seq;
    MockApplication app;

    QJsonObject pajlada;
    pajlada["id"] = "11148817";
    pajlada["login"] = "pajlada";
    pajlada["display_name"] = "pajlada";
    pajlada["created_at"] = "2010-03-17T11:50:53Z";
    pajlada["description"] = " ͡° ͜ʖ ͡°)";
    pajlada["profile_image_url"] =
        "https://static-cdn.jtvnw.net/jtv_user_pictures/"
        "cbe986e3-06ad-4506-a3aa-eb05466c839c-profile_image-300x300.png";

    QJsonObject testaccount420;
    testaccount420["id"] = "117166826";
    testaccount420["login"] = "testaccount_420";
    testaccount420["display_name"] = "테스트계정420";
    testaccount420["created_at"] = "2016-02-27T18:55:59Z";
    testaccount420["description"] = "";
    testaccount420["profile_image_url"] =
        "https://static-cdn.jtvnw.net/user-default-pictures-uv/"
        "ead5c8b2-a4c9-4724-b1dd-9f00b46cbd3d-profile_image-300x300.png";

    QJsonObject forsen;
    forsen["id"] = "22484632";
    forsen["login"] = "forsen";
    forsen["display_name"] = "Forsen";
    forsen["created_at"] = "2011-05-19T00:28:28Z";
    forsen["description"] =
        "Approach with caution! No roleplaying or tryharding allowed.";
    forsen["profile_image_url"] =
        "https://static-cdn.jtvnw.net/jtv_user_pictures/"
        "forsen-profile_image-48b43e1e4f54b5c8-300x300.png";

    std::shared_ptr<TwitchChannel> channel =
        std::make_shared<TwitchChannel>("pajlada");
    channel->setRoomId("11148817");

    StrictMock<mock::Helix> mockHelix;
    initializeHelix(&mockHelix);

    EXPECT_CALL(mockHelix, update).Times(1);
    EXPECT_CALL(mockHelix, loadBlocks).Times(1);

    auto account = std::make_shared<TwitchAccount>(
        testaccount420["login"].toString(), "token", "oauthclient",
        testaccount420["id"].toString());
    getApp()->getAccounts()->twitch.accounts.append(account);
    getApp()->getAccounts()->twitch.currentUsername =
        testaccount420["login"].toString();
    getApp()->getAccounts()->twitch.load();

    // Simple single-channel ban
    EXPECT_CALL(mockHelix, fetchUsers(QStringList{"11148817"},
                                      QStringList{"forsen"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(pajlada),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix,
                banUser(pajlada["id"].toString(), QString("117166826"),
                        forsen["id"].toString(), std::optional<int>{},
                        QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand("/ban forsen", channel, false);

    // Multi-channel ban
    EXPECT_CALL(mockHelix, fetchUsers(QStringList{"11148817"},
                                      QStringList{"forsen"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(pajlada),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, banUser(pajlada["id"].toString(),
                                   testaccount420["id"].toString(),
                                   forsen["id"].toString(),
                                   std::optional<int>{}, QString(""), _, _))
        .Times(1);

    EXPECT_CALL(mockHelix,
                fetchUsers(QStringList{},
                           QStringList{"forsen", "testaccount_420"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(testaccount420),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, banUser(testaccount420["id"].toString(),
                                   testaccount420["id"].toString(),
                                   forsen["id"].toString(),
                                   std::optional<int>{}, QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand(
        "/ban --channel id:11148817 --channel testaccount_420 forsen", channel,
        false);

    // ID-based ban
    EXPECT_CALL(mockHelix,
                banUser(pajlada["id"].toString(), QString("117166826"),
                        forsen["id"].toString(), std::optional<int>{},
                        QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand("/ban id:22484632", channel, false);

    // ID-based redirected ban
    EXPECT_CALL(mockHelix,
                banUser(testaccount420["id"].toString(), QString("117166826"),
                        forsen["id"].toString(), std::optional<int>{},
                        QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand(
        "/ban --channel id:117166826 id:22484632", channel, false);

    // name-based redirected ban
    EXPECT_CALL(mockHelix, fetchUsers(QStringList{"22484632"},
                                      QStringList{"testaccount_420"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(testaccount420),
                HelixUser(forsen),
            };
            success(users);
        });
    EXPECT_CALL(mockHelix,
                banUser(testaccount420["id"].toString(), QString("117166826"),
                        forsen["id"].toString(), std::optional<int>{},
                        QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand(
        "/ban --channel testaccount_420 id:22484632", channel, false);

    // Multi-channel timeout
    EXPECT_CALL(mockHelix, fetchUsers(QStringList{"11148817"},
                                      QStringList{"forsen"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(pajlada),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, banUser(pajlada["id"].toString(),
                                   testaccount420["id"].toString(),
                                   forsen["id"].toString(),
                                   std::optional<int>{600}, QString(""), _, _))
        .Times(1);

    EXPECT_CALL(mockHelix,
                fetchUsers(QStringList{},
                           QStringList{"forsen", "testaccount_420"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(testaccount420),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, banUser(testaccount420["id"].toString(),
                                   testaccount420["id"].toString(),
                                   forsen["id"].toString(),
                                   std::optional<int>{600}, QString(""), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand(
        "/timeout --channel id:11148817 --channel testaccount_420 forsen",
        channel, false);

    // Multi-channel unban
    EXPECT_CALL(mockHelix, fetchUsers(QStringList{"11148817"},
                                      QStringList{"forsen"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(pajlada),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, unbanUser(pajlada["id"].toString(),
                                     testaccount420["id"].toString(),
                                     forsen["id"].toString(), _, _))
        .Times(1);

    EXPECT_CALL(mockHelix,
                fetchUsers(QStringList{},
                           QStringList{"forsen", "testaccount_420"}, _, _))
        .WillOnce([=](auto, auto, auto success, auto) {
            std::vector<HelixUser> users{
                HelixUser(testaccount420),
                HelixUser(forsen),
            };
            success(users);
        });

    EXPECT_CALL(mockHelix, unbanUser(testaccount420["id"].toString(),
                                     testaccount420["id"].toString(),
                                     forsen["id"].toString(), _, _))
        .Times(1);

    getApp()->getCommands()->execCommand(
        "/unban --channel id:11148817 --channel testaccount_420 forsen",
        channel, false);
}

}  // namespace chatterino
