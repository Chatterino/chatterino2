#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/builtin/twitch/Ban.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "mocks/Channel.hpp"
#include "mocks/EmptyApplication.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

using namespace chatterino;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : settings(this->settingsDir.filePath("settings.json"))
    {
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    Settings settings;
    AccountController accounts;
};

}  // namespace

TEST(Commands, parseBanCommand)
{
    MockApplication app;

    std::shared_ptr<TwitchChannel> channel =
        std::make_shared<TwitchChannel>("forsen");
    CommandContext ctx{
        .words = {"/ban", "forsen"},
        .channel = channel,
        .twitchChannel = channel.get(),
    };
    auto oActions = commands::parseChannelAction(ctx, "/ban", "usage", false);

    ASSERT_TRUE(oActions.has_value()) << oActions.error();
    auto actions = *oActions;
    ASSERT_EQ(actions.size(), 1);
    ASSERT_EQ(actions.front().rawTarget, "forsen");
}
