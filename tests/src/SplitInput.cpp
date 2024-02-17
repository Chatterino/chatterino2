#include "widgets/splits/SplitInput.hpp"

#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QDebug>
#include <QString>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : windowManager(this->paths)
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

    Theme theme;
    HotkeyController hotkeys;
    Fonts fonts;
    Paths paths;
    WindowManager windowManager;
    AccountController accounts;
    CommandController commands;
    Emotes emotes;
};

class SplitInputFixture : public ::testing::Test
{
protected:
    SplitInputFixture()
        : split(new Split(nullptr))
        , input(this->split)
    {
    }

    MockApplication mockApplication;
    Split *split;
    SplitInput input;
};

}  // namespace

TEST_F(SplitInputFixture, Reply)
{
    ASSERT_EQ("", this->input.getInputText());
    this->input.setInputText("forsen");
    ASSERT_EQ("forsen", this->input.getInputText());
    auto *message = new Message();
    message->displayName = "xd";
    auto reply = MessagePtr(message);
    this->input.setReply(reply);
    QString expected("@xd forsen ");
    QString actual = this->input.getInputText();
    ASSERT_EQ(expected, actual)
        << "Input text after setReply should be '" << qUtf8Printable(expected)
        << "', but got '" << qUtf8Printable(actual) << "'";
}
