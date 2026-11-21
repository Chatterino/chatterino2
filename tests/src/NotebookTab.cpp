// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/NotebookTab.hpp"

#include "common/Literals.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "gmock/gmock.h"
#include "mocks/BaseApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"
#include "widgets/Notebook.hpp"

#include <QApplication>
#include <QDebug>
#include <QString>
#include <QWheelEvent>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : windowManager(this->args_, this->paths_, this->settings, this->theme,
                        this->fonts)
    {
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }
    HotkeyController *getHotkeys() override
    {
        return &this->hotkeys;
    }
    HotkeyController hotkeys;
    WindowManager windowManager;
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

class NotebookWheelFixture : public ::testing::Test
{
protected:
    NotebookWheelFixture()
        : notebook(nullptr)
    {
        this->notebook.setAttribute(Qt::WA_DontShowOnScreen);
        this->notebook.resize(500, 300);
        this->firstTab = this->notebook.addPage(new QWidget, "first", true);
        this->notebook.addPage(new QWidget, "second");
        this->notebook.show();
        QApplication::processEvents();
    }

    static void scroll(QWidget *target, QPoint position, int delta)
    {
        QWheelEvent event(position, target->mapToGlobal(position), {},
                          {0, delta}, Qt::NoButton, Qt::NoModifier,
                          Qt::NoScrollPhase, false);
        QApplication::sendEvent(target, &event);
    }

    MockApplication app;
    Notebook notebook;
    NotebookTab *firstTab{};
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

TEST_F(NotebookWheelFixture, ScrollOnTab)
{
    scroll(this->firstTab, this->firstTab->rect().center(), -120);
    EXPECT_EQ(this->notebook.getSelectedIndex(), 1);
}

TEST_F(NotebookWheelFixture, ScrollOnTabStripBackground)
{
    const QPoint position{this->notebook.width() - 5, 14};
    ASSERT_EQ(this->notebook.childAt(position), nullptr);

    scroll(&this->notebook, position, -120);
    EXPECT_EQ(this->notebook.getSelectedIndex(), 1);
}

TEST_F(NotebookWheelFixture, ScrollOverPageDoesNotChangeTab)
{
    const QPoint position{this->notebook.width() / 2,
                          this->notebook.height() / 2};
    ASSERT_TRUE(
        this->notebook.getSelectedPage()->geometry().contains(position));

    scroll(&this->notebook, position, -120);
    EXPECT_EQ(this->notebook.getSelectedIndex(), 0);
}

TEST_F(NotebookWheelFixture, SmallDeltasAccumulateAcrossTabStrip)
{
    const QPoint background{this->notebook.width() - 5, 14};
    ASSERT_EQ(this->notebook.childAt(background), nullptr);

    scroll(this->firstTab, this->firstTab->rect().center(), -60);
    EXPECT_EQ(this->notebook.getSelectedIndex(), 0);
    scroll(&this->notebook, background, -60);
    EXPECT_EQ(this->notebook.getSelectedIndex(), 1);
}
