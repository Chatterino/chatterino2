// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/Aliases.hpp"

#include "Test.hpp"

#include <boost/unordered/unordered_flat_map.hpp>

using namespace chatterino;
using namespace Qt::Literals;

template <typename T, typename U>
constexpr inline auto CAN_COMPARE_EQ =
    requires(const T &t, const U &u) { t == u; };

TEST(Aliases, Spaceship)
{
    // owned <=> owned
    {
        UserName a{u"user"_s};
        UserName b{"user"};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a <= b);
        EXPECT_TRUE(a >= b);
        EXPECT_TRUE(b == a);
        EXPECT_TRUE(b <= a);
        EXPECT_TRUE(b >= a);

        EXPECT_FALSE(b > a);
        EXPECT_FALSE(b < a);
        EXPECT_FALSE(a > b);
        EXPECT_FALSE(a < b);

        UserName c{u"x"_s};
        EXPECT_TRUE(c != a);
        EXPECT_TRUE(c > a);
        EXPECT_TRUE(c >= a);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a < c);
        EXPECT_TRUE(a <= c);

        EXPECT_FALSE(c == a);
        EXPECT_FALSE(c < a);
        EXPECT_FALSE(c <= a);
        EXPECT_FALSE(a == c);
        EXPECT_FALSE(a > c);
        EXPECT_FALSE(a >= c);
    }
    // view <=> view
    {
        EmoteNameView a{u"emote"};
        EmoteNameView b{u"emote"};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a <= b);
        EXPECT_TRUE(a >= b);
        EXPECT_TRUE(b == a);
        EXPECT_TRUE(b <= a);
        EXPECT_TRUE(b >= a);

        EXPECT_FALSE(b != a);
        EXPECT_FALSE(a != b);
        EXPECT_FALSE(b > a);
        EXPECT_FALSE(b < a);
        EXPECT_FALSE(a > b);
        EXPECT_FALSE(a < b);

        EmoteNameView c{u"other"};
        EXPECT_TRUE(c != a);
        EXPECT_TRUE(c > a);
        EXPECT_TRUE(c >= a);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a < c);
        EXPECT_TRUE(a <= c);

        EXPECT_FALSE(c == a);
        EXPECT_FALSE(c < a);
        EXPECT_FALSE(c <= a);
        EXPECT_FALSE(a == c);
        EXPECT_FALSE(a > c);
        EXPECT_FALSE(a >= c);
    }
    // owned <=> view
    {
        EmoteId a{u"a string"_s};
        EmoteIdView b{u"a string"};

        EXPECT_TRUE(a == b);
        EXPECT_TRUE(a <= b);
        EXPECT_TRUE(a >= b);
        EXPECT_TRUE(b == a);
        EXPECT_TRUE(b <= a);
        EXPECT_TRUE(b >= a);

        EXPECT_FALSE(b > a);
        EXPECT_FALSE(b < a);
        EXPECT_FALSE(a > b);
        EXPECT_FALSE(a < b);

        EmoteId c{u"another string"_s};
        EXPECT_TRUE(c != a);
        EXPECT_TRUE(c > a);
        EXPECT_TRUE(c >= a);
        EXPECT_TRUE(a != c);
        EXPECT_TRUE(a < c);
        EXPECT_TRUE(a <= c);

        EXPECT_FALSE(c == a);
        EXPECT_FALSE(c < a);
        EXPECT_FALSE(c <= a);
        EXPECT_FALSE(a == c);
        EXPECT_FALSE(a > c);
        EXPECT_FALSE(a >= c);
    }

    static_assert(CAN_COMPARE_EQ<EmoteName, EmoteName>);
    static_assert(CAN_COMPARE_EQ<EmoteName, EmoteNameView>);

    // "Raw" QStrings.
    static_assert(!CAN_COMPARE_EQ<EmoteName, QString>);
    static_assert(!CAN_COMPARE_EQ<EmoteName, const char *>);
    static_assert(!CAN_COMPARE_EQ<EmoteName, QStringView>);
    static_assert(!CAN_COMPARE_EQ<EmoteName, const char16_t *>);
    static_assert(!CAN_COMPARE_EQ<EmoteNameView, QString>);
    static_assert(!CAN_COMPARE_EQ<EmoteNameView, const char *>);
    static_assert(!CAN_COMPARE_EQ<EmoteNameView, QStringView>);
    static_assert(!CAN_COMPARE_EQ<EmoteNameView, const char16_t *>);

    // Unrelated types.
    static_assert(!CAN_COMPARE_EQ<EmoteName, EmoteId>);
    static_assert(!CAN_COMPARE_EQ<EmoteName, EmoteIdView>);
    static_assert(!CAN_COMPARE_EQ<EmoteName, UserNameView>);
}

TEST(Aliases, ContainerMap)
{
    // We need std::less<> here to unlock the transparent overloads for find and
    // friends. Otherwise, using EmoteIdView won't work for lookup.
    std::map<EmoteId, EmoteName, std::less<>> map;
    map.emplace(EmoteId{"foo"}, EmoteName{"bar"});
    map.emplace(EmoteId{u"baz"_s}, EmoteName{u"qox"_s});

    EXPECT_TRUE(map.contains(EmoteIdView{u"baz"}));
    EXPECT_TRUE(map.contains(EmoteId{u"foo"_s}));
    EXPECT_FALSE(map.contains(EmoteIdView{u"no in here"}));

    // Can't check at compile time that passing a raw QString(View) here will
    // fail, because `contains` isn't SFINAE friendly. The following two lines
    // should fail to compile when uncommented.
    // EXPECT_TRUE(map.contains("baz"));
    // EXPECT_TRUE(map.contains(u"baz"));
}

TEST(Aliases, ContainerUnorderedMap)
{
    // We need to use std::equal_to<> here to be able to use EmoteIdView for
    // lookup.
    std::unordered_map<EmoteId, EmoteName, EmoteIdHash, std::equal_to<>> map;
    map.emplace(EmoteId{"foo"}, EmoteName{"bar"});
    map.emplace(EmoteId{u"baz"_s}, EmoteName{u"qox"_s});

    EXPECT_TRUE(map.contains(EmoteIdView{u"baz"}));
    EXPECT_TRUE(map.contains(EmoteId{u"foo"_s}));
    EXPECT_FALSE(map.contains(EmoteIdView{u"no in here"}));

    // Can't check at compile time that passing a raw QString(View) here will
    // fail, because `contains` isn't SFINAE friendly. The following two lines
    // should fail to compile when uncommented.
    // EXPECT_TRUE(map.contains("baz"));
    // EXPECT_TRUE(map.contains(u"baz"));

    // Check that the map still works even if `std::equal_to<>` isn't used.
    // This should be avoided in practice.
    std::unordered_map<EmoteId, EmoteName> nonTransparent;
    nonTransparent.emplace(EmoteId{"foo"}, EmoteName{"bar"});
    nonTransparent.emplace(EmoteId{u"baz"_s}, EmoteName{u"qox"_s});
    EXPECT_TRUE(nonTransparent.contains(EmoteId{"foo"}));
    // Using `EmoteIdView` won't work.
}

TEST(Aliases, ContainerBoostUnorderedFlatMap)
{
    // We need to use std::equal_to<> here to be able to use EmoteIdView for
    // lookup.
    boost::unordered_flat_map<UserId, Tooltip, UserIdHash, std::equal_to<>> map;
    map.emplace(UserId{"foo"}, Tooltip{"bar"});
    map.emplace(UserId{u"baz"_s}, Tooltip{u"qox"_s});

    EXPECT_TRUE(map.contains(UserIdView{u"baz"}));
    EXPECT_TRUE(map.contains(UserId{u"foo"_s}));
    EXPECT_FALSE(map.contains(UserIdView{u"no in here"}));

    // Can't check at compile time that passing a raw QString(View) here will
    // fail, because `contains` isn't SFINAE friendly. The following two lines
    // should fail to compile when uncommented.
    // EXPECT_TRUE(map.contains("baz"));
    // EXPECT_TRUE(map.contains(u"baz"));

    // Check that the map still works even if `std::equal_to<>` isn't used.
    // This should be avoided in practice.
    boost::unordered_flat_map<UserId, Tooltip> nonTransparent;
    nonTransparent.emplace(UserId{"foo"}, Tooltip{"bar"});
    nonTransparent.emplace(UserId{u"baz"_s}, Tooltip{u"qox"_s});
    EXPECT_TRUE(nonTransparent.contains(UserId{"foo"}));
    // Using `UserIdView` won't work.
}
