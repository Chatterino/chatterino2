// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/TabHistory.hpp"

#include "Test.hpp"

#include <QWidget>

#include <memory>
#include <vector>

using namespace chatterino;

namespace {

class TabHistoryTest : public ::testing::Test
{
protected:
    QWidget parent;
};

}  // namespace

TEST_F(TabHistoryTest, RecordVisitBuildsBackStack)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    TabHistory history;

    EXPECT_FALSE(history.canGoBack());

    history.recordVisit(&a);
    EXPECT_TRUE(history.canGoBack());
    EXPECT_EQ(history.peekBack(), &a);

    history.recordVisit(&b);
    EXPECT_EQ(history.peekBack(), &b);
    EXPECT_EQ(history.backStackMostRecentFirst(),
              (std::vector<QWidget *>{&b, &a}));
}

TEST_F(TabHistoryTest, RecordVisitIgnoresNull)
{
    TabHistory history;

    history.recordVisit(nullptr);

    EXPECT_FALSE(history.canGoBack());
}

TEST_F(TabHistoryTest, GoBackAndForward)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordVisit(&a);
    history.recordVisit(&b);

    auto back = history.goBack(&c);
    ASSERT_TRUE(back.has_value());
    EXPECT_EQ(*back, &b);
    EXPECT_TRUE(history.canGoForward());
    EXPECT_EQ(history.peekForward(), &c);

    auto forward = history.goForward(&b);
    ASSERT_TRUE(forward.has_value());
    EXPECT_EQ(*forward, &c);
    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, GoBackWithoutCurrentSkipsForwardStack)
{
    QWidget a(&this->parent);
    TabHistory history;

    history.recordVisit(&a);

    auto back = history.goBack(nullptr);
    ASSERT_TRUE(back.has_value());
    EXPECT_EQ(*back, &a);
    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, RecordVisitClearsForwardStack)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordVisit(&a);
    history.recordVisit(&b);
    history.goBack(&c);
    ASSERT_TRUE(history.canGoForward());

    history.recordVisit(&b);

    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, RemovePage)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordVisit(&a);
    history.recordVisit(&b);
    history.removePage(&a);

    EXPECT_EQ(history.backStackMostRecentFirst(), (std::vector<QWidget *>{&b}));
}

TEST_F(TabHistoryTest, DiscardTopEntries)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordVisit(&a);
    history.recordVisit(&b);
    history.goBack(&c);

    history.discardBackTop();
    EXPECT_FALSE(history.canGoBack());

    history.discardForwardTop();
    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, TrimsOldestEntriesAtCapacity)
{
    std::vector<std::unique_ptr<QWidget>> pages;
    pages.reserve(52);
    for (int i = 0; i < 52; ++i)
    {
        pages.push_back(std::make_unique<QWidget>(&this->parent));
    }

    TabHistory history;
    for (int i = 0; i < 51; ++i)
    {
        history.recordVisit(pages[static_cast<size_t>(i)].get());
    }

    auto stack = history.backStackMostRecentFirst();
    ASSERT_EQ(stack.size(), 50U);
    EXPECT_EQ(stack.front(), pages[50].get());
    EXPECT_EQ(stack.back(), pages[1].get());
}
