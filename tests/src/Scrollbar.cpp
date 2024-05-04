#include "widgets/Scrollbar.hpp"

#include "Application.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <gtest/gtest.h>
#include <QString>

#include <memory>

using namespace chatterino;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : settings(this->settingsDir.filePath("settings.json"))
        , fonts(this->settings)
        , windowManager(this->paths_)
    {
    }
    Theme *getThemes() override
    {
        return &this->theme;
    }

    Fonts *getFonts() override
    {
        return &this->fonts;
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }

    Settings settings;
    Theme theme;
    Fonts fonts;
    WindowManager windowManager;
};

}  // namespace

TEST(Scrollbar, AddHighlight)
{
    MockApplication mockApplication;

    Scrollbar scrollbar(10, nullptr);
    EXPECT_EQ(scrollbar.getHighlights().size(), 0);

    for (int i = 0; i < 15; ++i)
    {
        auto color = std::make_shared<QColor>(i, 0, 0);
        ScrollbarHighlight scrollbarHighlight{color};
        scrollbar.addHighlight(scrollbarHighlight);
    }

    EXPECT_EQ(scrollbar.getHighlights().size(), 10);
    auto highlights = scrollbar.getHighlights();
    for (int i = 0; i < 10; ++i)
    {
        auto highlight = highlights[i];
        EXPECT_EQ(highlight.getColor().red(), i + 5);
    }
}
