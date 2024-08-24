#include "widgets/helper/NotebookTab.hpp"

#include "common/Literals.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "gmock/gmock.h"
#include "mocks/BaseApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "Test.hpp"
#include "widgets/Notebook.hpp"

#include <QDebug>
#include <QString>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : theme(this->paths_)
        , fonts(this->settings)
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

    Theme theme;
    HotkeyController hotkeys;
    Fonts fonts;
};

class MockNotebookTab : public NotebookTab
{
public:
    explicit MockNotebookTab(Notebook *notebook)
        : NotebookTab(notebook)
    {
    }

    MOCK_METHOD(void, update, (), (override));
};

class NotebookTabFixture : public ::testing::Test
{
protected:
    NotebookTabFixture()
        : notebook(nullptr)
        , tab(&this->notebook)
    {
    }

    MockApplication mockApplication;
    Notebook notebook;
    MockNotebookTab tab;
};

}  // namespace

/// The highlight state must settable
TEST_F(NotebookTabFixture, SetHighlightState)
{
    EXPECT_CALL(this->tab, update).Times(Exactly(1));
    EXPECT_EQ(this->tab.highlightState(), HighlightState::None);
    this->tab.setHighlightState(HighlightState::NewMessage);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::NewMessage);
}

/// The highlight state must be able to "upgrade" from NewMessage to Highlighted
TEST_F(NotebookTabFixture, UpgradeHighlightState)
{
    EXPECT_CALL(this->tab, update).Times(Exactly(2));
    EXPECT_EQ(this->tab.highlightState(), HighlightState::None);
    this->tab.setHighlightState(HighlightState::NewMessage);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::NewMessage);
    this->tab.setHighlightState(HighlightState::Highlighted);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::Highlighted);
}

/// The highlight state must stay as NewMessage when called twice
TEST_F(NotebookTabFixture, SameHighlightStateNewMessage)
{
    EXPECT_CALL(this->tab, update).Times(Exactly(1));
    EXPECT_EQ(this->tab.highlightState(), HighlightState::None);
    this->tab.setHighlightState(HighlightState::NewMessage);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::NewMessage);
    this->tab.setHighlightState(HighlightState::NewMessage);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::NewMessage);
}

/// The highlight state must stay as Highlighted when called twice, and must not call update more than once
TEST_F(NotebookTabFixture, SameHighlightStateHighlighted)
{
    EXPECT_CALL(this->tab, update).Times(Exactly(1));
    EXPECT_EQ(this->tab.highlightState(), HighlightState::None);
    this->tab.setHighlightState(HighlightState::Highlighted);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::Highlighted);
    this->tab.setHighlightState(HighlightState::Highlighted);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::Highlighted);
}

/// The highlight state must not downgrade from Highlighted to NewMessage
TEST_F(NotebookTabFixture, DontDowngradeHighlightState)
{
    EXPECT_CALL(this->tab, update).Times(Exactly(1));
    EXPECT_EQ(this->tab.highlightState(), HighlightState::None);
    this->tab.setHighlightState(HighlightState::Highlighted);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::Highlighted);
    this->tab.setHighlightState(HighlightState::NewMessage);
    EXPECT_EQ(this->tab.highlightState(), HighlightState::Highlighted);
}
