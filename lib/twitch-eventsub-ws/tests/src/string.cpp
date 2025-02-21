#include <gtest/gtest.h>
#include <QString>
#include <twitch-eventsub-ws/string.hpp>

#include <type_traits>

namespace {

constexpr const char *LONGEST_SSO = "mylongstringyesitslongwo";
constexpr const char *TOO_LONG = "mylongstringyesitslongwow";

constexpr const char *REALLY_LONG =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi lectus "
    "massa, efficitur non elit vel, faucibus pellentesque enim.";

}  // namespace

using String = chatterino::eventsub::lib::String;

static_assert(std::is_move_assignable_v<String> &&
              std::is_move_constructible_v<String> &&
              !std::is_copy_assignable_v<String> &&
              !std::is_copy_constructible_v<String> &&
              !std::is_trivially_copyable_v<String> &&
              !std::is_trivially_move_assignable_v<String> &&
              !std::is_trivially_copy_assignable_v<String> &&
              !std::is_trivially_move_constructible_v<String> &&
              !std::is_trivially_copy_constructible_v<String>);

TEST(String, Construction)
{
    {
        String s;
        ASSERT_TRUE(s.isInPlace());
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_TRUE(s.isEmpty());
        ASSERT_EQ(s.view(), "");
    }
    {
        String s("");
        ASSERT_TRUE(s.isInPlace());
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_TRUE(s.isEmpty());
        ASSERT_EQ(s.view(), "");
    }
    {
        String s("foo");
        ASSERT_TRUE(s.isInPlace());
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
        ASSERT_EQ(s.view(), "foo");
    }
    {
        String s(LONGEST_SSO);
        ASSERT_TRUE(s.isInPlace());
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
        ASSERT_EQ(s.view(), LONGEST_SSO);
    }
    {
        String s(TOO_LONG);
        ASSERT_FALSE(s.isInPlace());
        ASSERT_FALSE(s.isQt());
        ASSERT_TRUE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
        ASSERT_EQ(s.view(), TOO_LONG);
    }
}

TEST(String, ToQt)
{
    {
        String s;
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), "");
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_TRUE(s.isEmpty());
    }
    {
        String s("");
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), "");
        ASSERT_FALSE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_TRUE(s.isEmpty());
    }
    {
        String s("foo");
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), "foo");
        ASSERT_TRUE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
    }
    {
        String s(LONGEST_SSO);
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), LONGEST_SSO);
        ASSERT_TRUE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
    }
    {
        String s(TOO_LONG);
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), TOO_LONG);
        ASSERT_TRUE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
    }
    {
        String s(REALLY_LONG);
        ASSERT_FALSE(s.isQt());
        ASSERT_EQ(s.qt(), REALLY_LONG);
        ASSERT_TRUE(s.isQt());
        ASSERT_FALSE(s.isAlloc());
        ASSERT_FALSE(s.isEmpty());
    }
}

TEST(String, MoveCtor)
{
    {
        String u;
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        String v(std::move(u));
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        ASSERT_EQ(v.view(), "");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u("");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        String v(std::move(u));
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        ASSERT_EQ(v.view(), "");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u("foo");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), "foo");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_FALSE(v.isQt());
        ASSERT_TRUE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), LONGEST_SSO);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), TOO_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), REALLY_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(std::move(u));
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
}

TEST(String, MoveAssign)
{
    {
        String u;
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        ASSERT_EQ(v.view(), "");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u("");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        ASSERT_EQ(v.view(), "");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u("foo");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), "foo");
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_FALSE(v.isQt());
        ASSERT_TRUE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), LONGEST_SSO);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), LONGEST_SSO);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), TOO_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), TOO_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
    {
        String u(REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), REALLY_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v;
        ASSERT_FALSE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_TRUE(v.isEmpty());
        v = std::move(u);
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.view(), REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_TRUE(u.isEmpty());
        ASSERT_EQ(u.view(), "");
    }
}

TEST(String, QtLifetime)
{
    QString qt;
    {
        String s(REALLY_LONG);
        qt = s.qt();
        ASSERT_TRUE(!qt.isDetached());  // s holds the string too
    }
    ASSERT_TRUE(qt.isDetached());
}
