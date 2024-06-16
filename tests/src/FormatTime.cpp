#include "util/FormatTime.hpp"

#include "Test.hpp"

#include <chrono>

using namespace chatterino;
using namespace std::chrono_literals;

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
            << actual << " (" << input << ") did not match expected value "
            << expected;
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
            << actual << " (" << input << ") did not match expected value "
            << expected;
    }
}

TEST(FormatTime, chrono)
{
    struct TestCase {
        std::chrono::seconds input;
        QString expectedOutput;
    };

    std::vector<TestCase> tests{
        {
            0s,
            "",
        },
        {
            1337s,
            "22m 17s",
        },
        {
            {22min + 17s},
            "22m 17s",
        },
        {
            623452s,
            "7d 5h 10m 52s",
        },
        {
            8345s,
            "2h 19m 5s",
        },
        {
            314034s,
            "3d 15h 13m 54s",
        },
        {
            27s,
            "27s",
        },
        {
            34589s,
            "9h 36m 29s",
        },
        {
            9h + 36min + 29s,
            "9h 36m 29s",
        },
        {
            3659s,
            "1h 59s",
        },
        {
            1h + 59s,
            "1h 59s",
        },
        {
            1045345s,
            "12d 2h 22m 25s",
        },
        {
            86432s,
            "1d 32s",
        },
    };

    for (const auto &[input, expected] : tests)
    {
        const auto actual = formatTime(input);

        EXPECT_EQ(actual, expected)
            << actual << " did not match expected value " << expected;
    }
}
