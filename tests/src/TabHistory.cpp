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

TEST_F(TabHistoryTest, RecordNavigationBuildsBackStack)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    TabHistory history;

    EXPECT_FALSE(history.canGoBack());
    EXPECT_FALSE(history.canGoForward());
    EXPECT_EQ(history.peekForward(), std::nullopt);

    history.recordNavigation(nullptr, &a);
    EXPECT_FALSE(history.canGoBack());
    EXPECT_FALSE(history.canGoForward());
    EXPECT_EQ(history.peekForward(), std::nullopt);

    history.recordNavigation(&a, &b);
    EXPECT_TRUE(history.canGoBack());
    EXPECT_FALSE(history.canGoForward());
    EXPECT_EQ(history.peekBack(), &a);
    EXPECT_EQ(history.peekForward(), std::nullopt);
    EXPECT_EQ(history.backStackMostRecentFirst(), (std::vector<QWidget *>{&a}));
}

TEST_F(TabHistoryTest, RecordNavigationIgnoresNullDestination)
{
    QWidget a(&this->parent);
    TabHistory history;

    history.recordNavigation(&a, nullptr);

    EXPECT_FALSE(history.canGoBack());
}

TEST_F(TabHistoryTest, GoBackAndForward)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordNavigation(nullptr, &a);
    history.recordNavigation(&a, &b);
    history.recordNavigation(&b, &c);

    auto back = history.goBack();
    EXPECT_EQ(back, &b);
    EXPECT_TRUE(history.canGoForward());
    EXPECT_EQ(history.peekForward(), &c);

    auto forward = history.goForward();
    EXPECT_EQ(forward, &c);
    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, RecordNavigationClearsForwardStack)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordNavigation(nullptr, &a);
    history.recordNavigation(&a, &b);
    history.recordNavigation(&b, &c);
    history.goBack();
    ASSERT_TRUE(history.canGoForward());

    history.recordNavigation(&b, &b);

    EXPECT_FALSE(history.canGoForward());
}

TEST_F(TabHistoryTest, RemovePage)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordNavigation(nullptr, &a);
    history.recordNavigation(&a, &b);
    history.recordNavigation(&b, &c);
    history.removePage(&a);

    EXPECT_EQ(history.backStackMostRecentFirst(), (std::vector<QWidget *>{&b}));
    EXPECT_EQ(history.peekBack(), &b);
    EXPECT_FALSE(history.canGoForward());
    EXPECT_EQ(history.peekForward(), std::nullopt);
}

TEST_F(TabHistoryTest, RemovePageUpdatesPeekEntries)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordNavigation(nullptr, &a);
    history.recordNavigation(&a, &b);
    history.recordNavigation(&b, &c);
    history.goBack();

    EXPECT_EQ(history.peekBack(), &a);
    EXPECT_EQ(history.peekForward(), &c);

    history.removePage(&b);

    EXPECT_FALSE(history.canGoBack());
    EXPECT_EQ(history.peekForward(), &c);
}

TEST_F(TabHistoryTest, DiscardTopEntries)
{
    QWidget a(&this->parent);
    QWidget b(&this->parent);
    QWidget c(&this->parent);
    TabHistory history;

    history.recordNavigation(nullptr, &a);
    history.recordNavigation(&a, &b);
    history.recordNavigation(&b, &c);
    history.goBack();

    history.discardBackTop();
    EXPECT_FALSE(history.canGoBack());
    EXPECT_EQ(history.peekForward(), &c);

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
    history.recordNavigation(nullptr, pages[0].get());
    for (int i = 1; i < 52; ++i)
    {
        history.recordNavigation(pages[static_cast<size_t>(i - 1)].get(),
                                 pages[static_cast<size_t>(i)].get());
    }

    auto stack = history.backStackMostRecentFirst();
    ASSERT_EQ(stack.size(), 49U);
    EXPECT_EQ(stack.front(), pages[50].get());
    EXPECT_EQ(stack.back(), pages[2].get());
}
