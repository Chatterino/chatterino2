// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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

    // You can verify this with Temporal in your browser:
    // function humanDuration(from, to) {
    //   return Temporal.Instant.from(to)
    //     .since(Temporal.Instant.from(from))
    //     .round({
    //       smallestUnit: "seconds",
    //       largestUnit: "years",
    //       relativeTo: Temporal.Instant.from(from).toZonedDateTimeISO("UTC"),
    //     });
    // }
    // > humanDuration("2018-07-04T06:34:24Z", "2025-08-10T13:03:50Z")
    // > P7Y1M6DT6H29M26S
    std::array cases{
        // basic
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2025-08-10T13:03:50Z",
            .dur = "7 years, 1 month, 6 days, and 6 hours",
        },
        // eq
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:34:24Z",
            .dur = "0 seconds",
        },
        // one
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:34:25Z",
            .dur = "1 second",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:35:24Z",
            .dur = "1 minute",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:35:25Z",
            .dur = "1 minute and 1 second",
        },
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
            .dur = "1 month, 1 day, and 1 hour",
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
            .dur = "1 year, 1 day, and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-05T06:34:24Z",
            .dur = "1 year, 1 month, and 1 day",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-04T07:34:24Z",
            .dur = "1 year, 1 month, and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-08-05T07:34:24Z",
            .dur = "1 year, 1 month, 1 day, and 1 hour",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-05T07:35:24Z",
            .dur = "1 year, 1 day, 1 hour, and 1 minute",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2019-07-04T07:35:25Z",
            .dur = "1 year, 1 hour, 1 minute, and 1 second",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T07:35:25Z",
            .dur = "1 hour, 1 minute, and 1 second",
        },
        // two
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:34:26Z",
            .dur = "2 seconds",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:36:24Z",
            .dur = "2 minutes",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T06:36:26Z",
            .dur = "2 minutes and 2 seconds",
        },
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
            .dur = "2 months, 2 days, and 2 hours",
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
            .dur = "2 years, 2 months, and 2 days",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-09-06T08:34:24Z",
            .dur = "2 years, 2 months, 2 days, and 2 hours",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-07-06T08:36:24Z",
            .dur = "2 years, 2 days, 2 hours, and 2 minutes",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-07-04T08:36:26Z",
            .dur = "2 years, 2 hours, 2 minutes, and 2 seconds",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2018-07-04T08:36:26Z",
            .dur = "2 hours, 2 minutes, and 2 seconds",
        },
        // more units
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-09-06T08:36:26Z",
            .dur = "2 years, 2 months, 2 days, and 2 hours",
        },
        Case{
            .from = "2018-07-04T06:34:24Z",
            .to = "2020-07-06T08:36:26Z",
            .dur = "2 years, 2 days, 2 hours, and 2 minutes",
        },
        // swapped
        Case{
            .from = "2020-09-06T08:34:24Z",
            .to = "2018-07-04T06:34:24Z",
            .dur = "2 years, 2 months, 2 days, and 2 hours",
        },
        // lower time
        Case{
            .from = "2018-07-04T13:34:24Z",
            .to = "2025-08-04T10:03:50Z",
            .dur = "7 years, 30 days, 20 hours, and 29 minutes",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-07-05T00:03:00Z",
            .dur = "12 minutes and 10 seconds",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-07-05T00:00:00Z",
            .dur = "9 minutes and 10 seconds",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-07-06T00:03:00Z",
            .dur = "1 day, 12 minutes, and 10 seconds",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-07-07T00:03:00Z",
            .dur = "2 days, 12 minutes, and 10 seconds",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-08-04T00:03:00Z",
            .dur = "30 days, 12 minutes, and 10 seconds",
        },
        Case{
            .from = "2018-07-04T23:50:50Z",
            .to = "2018-09-04T00:03:00Z",
            .dur = "1 month, 30 days, 12 minutes, and 10 seconds",
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
