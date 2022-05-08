#include "util/Twitch.hpp"

#include <gtest/gtest.h>
#include <QApplication>
#include <QDebug>
#include <QtConcurrent>

#include <chrono>
#include <thread>

using namespace chatterino;

TEST(UtilTwitch, StripUserName)
{
    struct TestCase {
        QString inputUserName;
        QString expectedUserName;
    };

    std::vector<TestCase> tests{
        {
            "pajlada",
            "pajlada",
        },
        {
            "Pajlada",
            "Pajlada",
        },
        {
            "@Pajlada",
            "Pajlada",
        },
        {
            "@Pajlada,",
            "Pajlada",
        },
        {
            "@@Pajlada,",
            "@Pajlada",
        },
        {
            "@@Pajlada,,",
            "@Pajlada,",
        },
        {
            "",
            "",
        },
        {
            "@",
            "",
        },
        {
            ",",
            "",
        },
        {
            // We purposefully don't handle spaces at the end, as all expected usages of this function split the message up by space and strip the parameters by themselves
            ", ",
            ", ",
        },
        {
            // We purposefully don't handle spaces at the start, as all expected usages of this function split the message up by space and strip the parameters by themselves
            " @",
            " @",
        },
    };

    for (const auto &[inputUserName, expectedUserName] : tests)
    {
        QString userName = inputUserName;
        stripUserName(userName);

        EXPECT_EQ(userName, expectedUserName)
            << qUtf8Printable(userName) << " (" << qUtf8Printable(inputUserName)
            << ") did not match expected value "
            << qUtf8Printable(expectedUserName);
    }
}

TEST(UtilTwitch, StripChannelName)
{
    struct TestCase {
        QString inputChannelName;
        QString expectedChannelName;
    };

    std::vector<TestCase> tests{
        {
            "pajlada",
            "pajlada",
        },
        {
            "Pajlada",
            "Pajlada",
        },
        {
            "@Pajlada",
            "Pajlada",
        },
        {
            "#Pajlada",
            "Pajlada",
        },
        {
            "#Pajlada,",
            "Pajlada",
        },
        {
            "#Pajlada,",
            "Pajlada",
        },
        {
            "@@Pajlada,",
            "@Pajlada",
        },
        {
            // We only strip one character off the front
            "#@Pajlada,",
            "@Pajlada",
        },
        {
            "@@Pajlada,,",
            "@Pajlada,",
        },
        {
            "",
            "",
        },
        {
            "@",
            "",
        },
        {
            ",",
            "",
        },
        {
            // We purposefully don't handle spaces at the end, as all expected usages of this function split the message up by space and strip the parameters by themselves
            ", ",
            ", ",
        },
        {
            // We purposefully don't handle spaces at the start, as all expected usages of this function split the message up by space and strip the parameters by themselves
            " #",
            " #",
        },
    };

    for (const auto &[inputChannelName, expectedChannelName] : tests)
    {
        QString userName = inputChannelName;
        stripChannelName(userName);

        EXPECT_EQ(userName, expectedChannelName)
            << qUtf8Printable(userName) << " ("
            << qUtf8Printable(inputChannelName)
            << ") did not match expected value "
            << qUtf8Printable(expectedChannelName);
    }
}

TEST(UtilTwitch, UserLoginRegexp)
{
    struct TestCase {
        QString inputUserLogin;
        bool matches;
    };

    std::vector<TestCase> tests{
        {
            "pajlada",
            true,
        },
        {
            // Login names must not start with an underscore
            "_pajlada",
            false,
        },
        {
            "@pajlada",
            false,
        },
        {
            "pajlada!",
            false,
        },
        {
            "pajlada,",
            false,
        },
        {
            // Login names must not contain capital letters
            "Pajlada",
            false,
        },
        {"k", true},
        {"testaccount_420", true},
        {"3v", true},
        {"ron", true},
        {"bits", true},
    };

    const auto &regexp = twitchUserLoginRegexp();

    for (const auto &[inputUserLogin, expectedMatch] : tests)
    {
        const auto &match = regexp.match(inputUserLogin);
        auto actual = regexp.match(inputUserLogin);

        EXPECT_EQ(match.hasMatch(), expectedMatch)
            << qUtf8Printable(inputUserLogin) << " did not match as expected";
    }
}

TEST(UtilTwitch, UserNameRegexp)
{
    struct TestCase {
        QString inputUserLogin;
        bool matches;
    };

    std::vector<TestCase> tests{
        {"PAJLADA", true},
        {
            // User names must not start with an underscore
            "_pajlada",
            false,
        },
        {
            "@pajlada",
            false,
        },
        {
            "pajlada!",
            false,
        },
        {
            "pajlada,",
            false,
        },
        {
            "Pajlada",
            true,
        },
        {"k", true},
        {"testaccount_420", true},
        {"3v", true},
        {"ron", true},
        {"bits", true},
        {"3V", true},
        {"Ron", true},
        {"Bits", true},
    };

    const auto &regexp = twitchUserNameRegexp();

    for (const auto &[inputUserLogin, expectedMatch] : tests)
    {
        const auto &match = regexp.match(inputUserLogin);
        auto actual = regexp.match(inputUserLogin);

        EXPECT_EQ(match.hasMatch(), expectedMatch)
            << qUtf8Printable(inputUserLogin) << " did not match as expected";
    }
}
