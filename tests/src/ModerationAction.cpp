#include "controllers/moderationactions/ModerationAction.hpp"

#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/Image.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"

#include <gtest/gtest.h>
#include <QString>

using namespace chatterino;

using namespace std::chrono_literals;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : settings(this->settingsDir.filePath("settings.json"))
        , fonts(this->settings)
        , windowManager(this->paths)
    {
    }
    Theme *getThemes() override
    {
        return &this->theme;
    }

    HotkeyController *getHotkeys() override
    {
        return &this->hotkeys;
    }

    Fonts *getFonts() override
    {
        return &this->fonts;
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
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

    Settings settings;
    Theme theme;
    HotkeyController hotkeys;
    Fonts fonts;
    Paths paths;
    WindowManager windowManager;
    AccountController accounts;
    CommandController commands;
    Emotes emotes;
};

class ModerationActionTest : public ::testing::Test
{
public:
    MockApplication mockApplication;
};

}  // namespace

TEST_F(ModerationActionTest, Parse)
{
    struct TestCase {
        QString action;

        QString expectedLine1;
        QString expectedLine2;

        std::optional<ImagePtr> expectedImage;
    };

    std::vector<TestCase> tests{
        {
            .action = "/ban forsen",
            .expectedImage =
                Image::fromResourcePixmap(getResources().buttons.ban),
        },
        {
            .action = "/delete {message.id}",
            .expectedImage =
                Image::fromResourcePixmap(getResources().buttons.trashCan),
        },
        {
            .action = "/timeout {user.name} 1d",
            .expectedLine1 = "1",
            .expectedLine2 = "d",
        },
        {
            .action = ".timeout {user.name} 300",
            .expectedLine1 = "5",
            .expectedLine2 = "m",
        },
        {
            .action = "forsen",
            .expectedLine1 = "fo",
            .expectedLine2 = "rs",
        },
    };

    for (const auto &test : tests)
    {
        ModerationAction moderationAction(test.action);

        EXPECT_EQ(moderationAction.getAction(), test.action);

        EXPECT_EQ(moderationAction.getLine1(), test.expectedLine1);
        EXPECT_EQ(moderationAction.getLine2(), test.expectedLine2);

        if (test.expectedImage.has_value())
        {
            EXPECT_EQ(moderationAction.getImage(), test.expectedImage);
        }
    }
}
