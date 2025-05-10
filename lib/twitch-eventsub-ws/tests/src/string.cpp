#include <gtest/gtest.h>
#include <QString>
#include <QStringBuilder>
#include <twitch-eventsub-ws/string.hpp>

#include <type_traits>

namespace {

constexpr const char *MINI = "mini";
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
    {
        String u(REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), REALLY_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(REALLY_LONG);
        ASSERT_FALSE(v.isQt());
        ASSERT_TRUE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
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
    {
        String u(REALLY_LONG);
        ASSERT_FALSE(u.isQt());
        ASSERT_TRUE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        ASSERT_EQ(u.qt(), REALLY_LONG);
        ASSERT_TRUE(u.isQt());
        ASSERT_FALSE(u.isAlloc());
        ASSERT_FALSE(u.isEmpty());
        String v(REALLY_LONG);
        ASSERT_FALSE(v.isQt());
        ASSERT_TRUE(v.isAlloc());
        ASSERT_FALSE(v.isEmpty());
        ASSERT_EQ(v.qt(), REALLY_LONG);
        ASSERT_TRUE(v.isQt());
        ASSERT_FALSE(v.isAlloc());
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
        ASSERT_FALSE(qt.isDetached());  // s holds the string too
    }
    ASSERT_TRUE(qt.isDetached());

    // move assignments
    {
        String s(REALLY_LONG);
        qt = s.qt();
        ASSERT_FALSE(qt.isDetached());
        s = {};
        ASSERT_TRUE(qt.isDetached());
    }
    ASSERT_TRUE(qt.isDetached());

    {
        String s(REALLY_LONG);
        qt = s.qt();
        ASSERT_FALSE(qt.isDetached());
        s = {LONGEST_SSO};
        ASSERT_TRUE(qt.isDetached());
    }
    ASSERT_TRUE(qt.isDetached());

    {
        String s(REALLY_LONG);
        qt = s.qt();
        ASSERT_FALSE(qt.isDetached());
        s = {REALLY_LONG};
        ASSERT_TRUE(qt.isDetached());
    }
    ASSERT_TRUE(qt.isDetached());
}

TEST(String, Equals)
{
    String empty;

    // This is using ASSERT_TRUE instead of ASSERT_EQ to be able to view/go to
    // the chosen operator== in editors (with clangd for example).
    ASSERT_TRUE(empty == empty);
    ASSERT_TRUE(empty == "");
    ASSERT_TRUE(empty == u"");
    ASSERT_TRUE(empty == std::string_view{});
    ASSERT_TRUE(empty == QAnyStringView());
    ASSERT_TRUE(empty == String());
    ASSERT_TRUE("" == empty);
    ASSERT_TRUE(u"" == empty);

    String longestSso(LONGEST_SSO);
    String longestSsoQt(LONGEST_SSO);
    ASSERT_EQ(longestSsoQt.qt(), LONGEST_SSO);

    ASSERT_TRUE(longestSso == longestSso);
    ASSERT_TRUE(longestSsoQt == longestSsoQt);
    ASSERT_TRUE(longestSso == longestSsoQt);
    ASSERT_TRUE(longestSso == LONGEST_SSO);
    ASSERT_TRUE(longestSsoQt == LONGEST_SSO);
    ASSERT_TRUE(longestSso == String(LONGEST_SSO));
    ASSERT_TRUE(longestSsoQt == String(LONGEST_SSO));

    String tooLong(TOO_LONG);
    String tooLongQt(TOO_LONG);
    ASSERT_EQ(tooLongQt.qt(), TOO_LONG);

    ASSERT_TRUE(tooLong == tooLong);
    ASSERT_TRUE(tooLongQt == tooLongQt);
    ASSERT_TRUE(tooLong == tooLongQt);
    ASSERT_TRUE(tooLong == TOO_LONG);
    ASSERT_TRUE(tooLongQt == TOO_LONG);
    ASSERT_TRUE(tooLong == String(TOO_LONG));
    ASSERT_TRUE(tooLongQt == String(TOO_LONG));

    String reallyLong(REALLY_LONG);
    String reallyLongQt(REALLY_LONG);
    ASSERT_EQ(reallyLongQt.qt(), REALLY_LONG);

    ASSERT_TRUE(reallyLong == reallyLong);
    ASSERT_TRUE(reallyLongQt == reallyLongQt);
    ASSERT_TRUE(reallyLong == reallyLongQt);
    ASSERT_TRUE(reallyLong == REALLY_LONG);
    ASSERT_TRUE(reallyLongQt == REALLY_LONG);
    ASSERT_TRUE(reallyLong == String(REALLY_LONG));
    ASSERT_TRUE(reallyLongQt == String(REALLY_LONG));
}

TEST(String, QStringBuilder)
{
    String s(MINI);

    ASSERT_EQ("foo" % s.qt() % "bar", "foominibar");
    ASSERT_EQ(u"foo" % s.qt() % "bar", "foominibar");
    ASSERT_EQ("foo" % s.qt() % u"bar", "foominibar");
    ASSERT_EQ(u"foo" % s.qt() % u"bar", "foominibar");

    // the code below breaks on qt 6.4.3, but i think it's unrelated to our String stuff
    // auto text2 = u"a" % s.qt() % u"b";
    // QString messageText = text2;
    // QString searchText = text2;
}
