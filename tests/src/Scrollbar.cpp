#include "widgets/Scrollbar.hpp"

#include "Application.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <QString>

#include <memory>

using namespace chatterino;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : windowManager(this->paths_, this->settings, this->theme, this->fonts)
    {
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }

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

TEST(Scrollbar, AddHighlightsAtStart)
{
    MockApplication mockApplication;

    Scrollbar scrollbar(10, nullptr);
    EXPECT_EQ(scrollbar.getHighlights().size(), 0);

    {
        scrollbar.addHighlightsAtStart({
            {
                std::make_shared<QColor>(1, 0, 0),
            },
        });
        auto highlights = scrollbar.getHighlights();
        EXPECT_EQ(highlights.size(), 1);
        EXPECT_EQ(highlights[0].getColor().red(), 1);
    }

    {
        scrollbar.addHighlightsAtStart({
            {
                std::make_shared<QColor>(2, 0, 0),
            },
        });
        auto highlights = scrollbar.getHighlights();
        EXPECT_EQ(highlights.size(), 2);
        EXPECT_EQ(highlights[0].getColor().red(), 2);
        EXPECT_EQ(highlights[1].getColor().red(), 1);
    }

    {
        scrollbar.addHighlightsAtStart({
            {
                std::make_shared<QColor>(4, 0, 0),
            },
            {
                std::make_shared<QColor>(3, 0, 0),
            },
        });
        auto highlights = scrollbar.getHighlights();
        EXPECT_EQ(highlights.size(), 4);
        EXPECT_EQ(highlights[0].getColor().red(), 4);
        EXPECT_EQ(highlights[1].getColor().red(), 3);
        EXPECT_EQ(highlights[2].getColor().red(), 2);
        EXPECT_EQ(highlights[3].getColor().red(), 1);
    }

    {
        // Adds as many as it can, in reverse order
        scrollbar.addHighlightsAtStart({
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(10, 0, 0)},
            {std::make_shared<QColor>(9, 0, 0)},
            {std::make_shared<QColor>(8, 0, 0)},
            {std::make_shared<QColor>(7, 0, 0)},
            {std::make_shared<QColor>(6, 0, 0)},
            {std::make_shared<QColor>(5, 0, 0)},
        });
        auto highlights = scrollbar.getHighlights();
        EXPECT_EQ(highlights.size(), 10);
        for (const auto &highlight : highlights)
        {
            std::cout << highlight.getColor().red() << '\n';
        }
        EXPECT_EQ(highlights[0].getColor().red(), 10);
        EXPECT_EQ(highlights[1].getColor().red(), 9);
        EXPECT_EQ(highlights[2].getColor().red(), 8);
        EXPECT_EQ(highlights[3].getColor().red(), 7);
        EXPECT_EQ(highlights[4].getColor().red(), 6);
        EXPECT_EQ(highlights[5].getColor().red(), 5);
        EXPECT_EQ(highlights[6].getColor().red(), 4);
        EXPECT_EQ(highlights[7].getColor().red(), 3);
        EXPECT_EQ(highlights[8].getColor().red(), 2);
        EXPECT_EQ(highlights[9].getColor().red(), 1);
    }

    {
        // Adds as many as it can, in reverse order
        // Since the highlights are already full, nothing will be added
        scrollbar.addHighlightsAtStart({
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
            {std::make_shared<QColor>(255, 0, 0)},
        });
        auto highlights = scrollbar.getHighlights();
        EXPECT_EQ(highlights.size(), 10);
        for (const auto &highlight : highlights)
        {
            std::cout << highlight.getColor().red() << '\n';
        }
        EXPECT_EQ(highlights[0].getColor().red(), 10);
        EXPECT_EQ(highlights[1].getColor().red(), 9);
        EXPECT_EQ(highlights[2].getColor().red(), 8);
        EXPECT_EQ(highlights[3].getColor().red(), 7);
        EXPECT_EQ(highlights[4].getColor().red(), 6);
        EXPECT_EQ(highlights[5].getColor().red(), 5);
        EXPECT_EQ(highlights[6].getColor().red(), 4);
        EXPECT_EQ(highlights[7].getColor().red(), 3);
        EXPECT_EQ(highlights[8].getColor().red(), 2);
        EXPECT_EQ(highlights[9].getColor().red(), 1);
    }
}
