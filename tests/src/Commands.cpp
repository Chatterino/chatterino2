#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/common/ChannelAction.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

#include <QStringBuilder>

using namespace chatterino;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : settings(this->settingsDir.filePath("settings.json"))
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

    Settings settings;
    AccountController accounts;
    mock::MockTwitchIrcServer twitch;
};

}  // namespace

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
            // Normal ban with an added reason, with the user maybe trying to use the --channal parameter at the end, but it gets eaten by the reason
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
