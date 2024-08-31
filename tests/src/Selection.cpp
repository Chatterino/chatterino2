#include "messages/Selection.hpp"

#include "Test.hpp"

using namespace chatterino;

TEST(Selection, SelectionItemEmpty)
{
    auto a = SelectionItem{};
    ASSERT_EQ(a.charIndex, 0);
    ASSERT_EQ(a.messageIndex, 0);
}

TEST(Selection, SelectionItemLt)
{
    auto a = SelectionItem{1, 5};
    ASSERT_FALSE(a < a);

    auto b = SelectionItem{1, 10};
    ASSERT_LT(a, b);

    auto c = SelectionItem{20, 0};
    ASSERT_LT(a, c);
}

TEST(Selection, SelectionItemGt)
{
    auto a = SelectionItem{20, 0};
    ASSERT_FALSE(a > a);

    auto b = SelectionItem{1, 5};
    ASSERT_GT(a, b);

    auto c = SelectionItem{1, 10};
    ASSERT_GT(a, c);
}

TEST(Selection, SelectionItemEq)
{
    auto a = SelectionItem{1, 2};
    auto b = SelectionItem{1, 2};

    ASSERT_EQ(a, b);
    ASSERT_FALSE(a != b);

    auto c = SelectionItem{2, 1};
    auto d = SelectionItem{1, 0};

    ASSERT_NE(a, d);
    ASSERT_FALSE(a == c);
}

TEST(Selection, SelectionEmpty)
{
    auto a = Selection{};
    ASSERT_TRUE(a.isEmpty());
}

TEST(Selection, SelectionEq)
{
    auto a = Selection{{5, 5}, {10, 10}};
    auto b = Selection{{5, 5}, {10, 10}};

    ASSERT_EQ(a, b);
    ASSERT_FALSE(a != b);

    auto c = Selection{{15, 15}, {5, 5}};
    auto d = Selection{{10, 10}, {5, 5}};

    ASSERT_NE(a, d);
    ASSERT_FALSE(a == c);
}

TEST(Selection, SelectionMinMax)
{
    auto a = Selection{{1, 1}, {1, 2}};
    ASSERT_EQ(a.selectionMin, a.start);
    ASSERT_EQ(a.selectionMax, a.end);

    auto b = Selection{{2, 0}, {1, 10}};
    ASSERT_EQ(b.selectionMin, b.end);
    ASSERT_EQ(b.selectionMax, b.start);
}

TEST(Selection, SelectionUnion)
{
    {
        auto a = Selection{{1, 1}, {1, 10}};
        auto b = Selection{{1, 20}, {1, 30}};
        auto ab = a | b;

        ASSERT_EQ(ab.start, a.start);
        ASSERT_EQ(ab.end, b.end);
    }

    {
        auto a = Selection{{10, 0}, {5, 50}};
        auto b = Selection{{20, 0}, {25, 25}};
        auto ab = a | b;

        ASSERT_EQ(ab.start, a.end);
        ASSERT_EQ(ab.end, b.end);
    }

    {
        auto a = Selection{{3, 3}, {2, 1}};
        auto b = Selection{{1, 0}, {1, 10}};
        auto ab = a | b;

        ASSERT_EQ(ab.start, b.start);
        ASSERT_EQ(ab.end, a.start);

        auto ba = b | a;
        ASSERT_EQ(ab, ba);
    }
}

TEST(Selection, SelectionSingleMessage)
{
    auto a = Selection{{1, 1}, {1, 2}};
    ASSERT_TRUE(a.isSingleMessage());

    auto b = Selection{{1, 1}, {2, 2}};
    ASSERT_FALSE(b.isSingleMessage());
}

TEST(Selection, ShiftMessageIndex)
{
    auto a = Selection{{100, 1}, {200, 2}};

    a.shiftMessageIndex(10);
    ASSERT_EQ(a.selectionMin, a.start);
    ASSERT_EQ(a.selectionMin.charIndex, 1);
    ASSERT_EQ(a.selectionMin.messageIndex, 90);
    ASSERT_EQ(a.selectionMax, a.end);
    ASSERT_EQ(a.selectionMax.charIndex, 2);
    ASSERT_EQ(a.selectionMax.messageIndex, 190);

    a.shiftMessageIndex(20);
    ASSERT_EQ(a.selectionMin, a.start);
    ASSERT_EQ(a.selectionMin.messageIndex, 70);
    ASSERT_EQ(a.selectionMax, a.end);
    ASSERT_EQ(a.selectionMax.messageIndex, 170);

    a.shiftMessageIndex(180);
    ASSERT_EQ(a.selectionMin.messageIndex, 0);
    ASSERT_EQ(a.selectionMax.messageIndex, 0);
    ASSERT_EQ(a.start.messageIndex, 0);
    ASSERT_EQ(a.end.messageIndex, 0);
    ASSERT_EQ(a.start.charIndex, 0);
    ASSERT_EQ(a.end.charIndex, 0);
}
