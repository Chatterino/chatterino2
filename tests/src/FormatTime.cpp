#include "util/FormatTime.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

TEST(FormatTime, Int)
{
    struct TestCase {
        int input;
        QString expectedOutput;
    };

    std::vector<TestCase> tests{
        {
            0,
            "",
        },
        {
            1337,
            "22m 17s",
        },
        {
            623452,
            "7d 5h 10m 52s",
        },
        {
            8345,
            "2h 19m 5s",
        },
        {
            314034,
            "3d 15h 13m 54s",
        },
        {
            27,
            "27s",
        },
        {
            34589,
            "9h 36m 29s",
        },
        {
            3659,
            "1h 59s",
        },
        {
            1045345,
            "12d 2h 22m 25s",
        },
        {
            86432,
            "1d 32s",
        },
    };

    for (const auto &[input, expected] : tests)
    {
        const auto actual = formatTime(input);

        EXPECT_EQ(actual, expected)
            << qUtf8Printable(actual) << " (" << input
            << ") did not match expected value " << qUtf8Printable(expected);
    }
}

TEST(FormatTime, QString)
{
    struct TestCase {
        QString input;
        QString expectedOutput;
    };

    std::vector<TestCase> tests{
        {
            "0",
            "",
        },
        {
            "1337",
            "22m 17s",
        },
        {
            "623452",
            "7d 5h 10m 52s",
        },
        {
            "8345",
            "2h 19m 5s",
        },
        {
            "314034",
            "3d 15h 13m 54s",
        },
        {
            "27",
            "27s",
        },
        {
            "34589",
            "9h 36m 29s",
        },
        {
            "3659",
            "1h 59s",
        },
        {
            "1045345",
            "12d 2h 22m 25s",
        },
        {
            "86432",
            "1d 32s",
        },
        {
            "",
            "n/a",
        },
        {
            "asd",
            "n/a",
        },
    };

    for (const auto &[input, expected] : tests)
    {
        const auto actual = formatTime(input);

        EXPECT_EQ(actual, expected)
            << qUtf8Printable(actual) << " (" << qUtf8Printable(input)
            << ") did not match expected value " << qUtf8Printable(expected);
    }
}
