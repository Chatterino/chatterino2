#include "util/IrcHelpers.hpp"

#include <gtest/gtest.h>
#include <QApplication>
#include <QDebug>
#include <QtConcurrent>

#include <chrono>
#include <thread>

using namespace chatterino;

TEST(IrcHelpers, ParseTagString)
{
    struct TestCase {
        QString input;
        QString expected;
    };

    std::vector<TestCase> tests{
        {
            // No space escapes (normal string)
            R"(DefectiveCloak gifted a Tier 1 sub to aliiscrying!)",
            "DefectiveCloak gifted a Tier 1 sub to aliiscrying!",
        },
        {
            // space at end
            R"(DefectiveCloak\s\sgifted\sa\sTier\s1\ssub\sto\s)",
            "DefectiveCloak  gifted a Tier 1 sub to ",
        },
        {
            // consecutive spaces
            R"(DefectiveCloak\s\sgifted\sa\sTier\s1\ssub\sto\saliiscrying!)",
            "DefectiveCloak  gifted a Tier 1 sub to aliiscrying!",
        },
        {
            // non-consecutive spaces
            R"(DefectiveCloak\sgifted\sa\sTier\s1\ssub\sto\saliiscrying!)",
            "DefectiveCloak gifted a Tier 1 sub to aliiscrying!",
        },
        {
            // colon to semicolon
            R"(foo\:bar)",
            "foo;bar",
        },
        {
            // backslash
            R"(foo\\bar)",
            R"(foo\bar)",
        },
    };

    for (const auto &[input, expected] : tests)
    {
        const auto actual = parseTagString(input);

        EXPECT_EQ(actual, expected)
            << qUtf8Printable(actual) << " (" << qUtf8Printable(input)
            << ") did not match expected value " << qUtf8Printable(expected);
    }
}
