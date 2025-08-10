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

TEST(FormatTime, formatLongFriendlyDuration)
{
    struct Case {
        QString from;
        QString to;
        QString dur;
    };

    std::array cases{
        // basic
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2025-08-10T13:03:50Z",
            .dur = "7 years, 1 month, 6 days and 6 hours",
        },
        // eq
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:34:24Z",
            .dur = "0 hours",
        },
        // one
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T07:34:24Z",
            .dur = "1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-05T06:34:24Z",
            .dur = "1 day",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-05T07:34:24Z",
            .dur = "1 day and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-08-04T06:34:24Z",
            .dur = "1 month",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-08-05T06:34:24Z",
            .dur = "1 month and 1 day",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-08-05T07:34:24Z",
            .dur = "1 month, 1 day and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-08-04T07:34:24Z",
            .dur = "1 month and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-04T06:34:24Z",
            .dur = "1 year",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-04T07:34:24Z",
            .dur = "1 year and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-05T06:34:24Z",
            .dur = "1 year and 1 day",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-04T06:34:24Z",
            .dur = "1 year and 1 month",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-05T07:34:24Z",
            .dur = "1 year, 1 day and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-05T06:34:24Z",
            .dur = "1 year, 1 month and 1 day",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-04T07:34:24Z",
            .dur = "1 year, 1 month and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-05T07:34:24Z",
            .dur = "1 year, 1 month, 1 day and 1 hour",
        },
        // two
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T08:34:24Z",
            .dur = "2 hours",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-06T06:34:24Z",
            .dur = "2 days",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-06T08:34:24Z",
            .dur = "2 days and 2 hours",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-09-04T06:34:24Z",
            .dur = "2 months",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-09-06T06:34:24Z",
            .dur = "2 months and 2 days",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-09-06T08:34:24Z",
            .dur = "2 months, 2 days and 2 hours",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-07-04T06:34:24Z",
            .dur = "2 years",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-09-04T06:34:24Z",
            .dur = "2 years and 2 months",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-09-06T06:34:24Z",
            .dur = "2 years, 2 months and 2 days",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-09-06T08:34:24Z",
            .dur = "2 years, 2 months, 2 days and 2 hours",
        },
        // swapped
        Case{
            .from = "2020-09-06T08:34:24Z",
            .to = "2018-07-04T06:34:24Z",
            .dur = "2 years, 2 months, 2 days and 2 hours",
        },
    };

    for (const auto &c : cases)
    {
        auto d = formatLongFriendlyDuration(
            QDateTime::fromString(c.from, Qt::ISODate),
            QDateTime::fromString(c.to, Qt::ISODate));
        ASSERT_EQ(d, c.dur)
            << "from=" << c.from << " to=" << c.to << " expected=" << c.dur;
    }
}
