#include "util/Helpers.hpp"

#include "mocks/BaseApplication.hpp"
#include "Test.hpp"

#include <QDateTime>
#include <QTimeZone>

#include <span>

using namespace chatterino;
using namespace helpers::detail;

TEST(Helpers, formatUserMention)
{
    const auto userName = "pajlada";

    // A user mention that is the first word, that has 'mention with comma' enabled should have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, true, true), "pajlada,");

    // A user mention that is not the first word, but has 'mention with comma' enabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, false, true), "pajlada");

    // A user mention that is the first word, but has 'mention with comma' disabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, true, false), "pajlada");

    // A user mention that is neither the first word, nor has 'mention with comma' enabled should not have a comma appended at the end.
    EXPECT_EQ(formatUserMention(userName, false, false), "pajlada");
}

TEST(Helpers, BatchTwoParts)
{
    QStringList input{
        "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "10",
        "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",  "20",
        "21",  "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",  "30",
        "31",  "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",  "40",
        "41",  "42",  "43",  "44",  "45",  "46",  "47",  "48",  "49",  "50",
        "51",  "52",  "53",  "54",  "55",  "56",  "57",  "58",  "59",  "60",
        "61",  "62",  "63",  "64",  "65",  "66",  "67",  "68",  "69",  "70",
        "71",  "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",  "80",
        "81",  "82",  "83",  "84",  "85",  "86",  "87",  "88",  "89",  "90",
        "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",  "99",  "100",
        "101", "102", "103", "104", "105", "106", "107", "108", "109", "110",
        "111", "112", "113", "114", "115", "116", "117", "118", "119", "120",
        "121", "122", "123", "124", "125", "126", "127", "128", "129", "130",
        "131", "132", "133", "134", "135", "136", "137", "138", "139", "140",
        "141", "142", "143", "144", "145", "146", "147", "148", "149", "150",
    };

    std::vector<QStringList> expectation = {
        {
            "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10",
            "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
            "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
            "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
            "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
            "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
            "61", "62", "63", "64", "65", "66", "67", "68", "69", "70",
            "71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
            "81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
            "91", "92", "93", "94", "95", "96", "97", "98", "99", "100",
        },
        {
            "101", "102", "103", "104", "105", "106", "107", "108", "109",
            "110", "111", "112", "113", "114", "115", "116", "117", "118",
            "119", "120", "121", "122", "123", "124", "125", "126", "127",
            "128", "129", "130", "131", "132", "133", "134", "135", "136",
            "137", "138", "139", "140", "141", "142", "143", "144", "145",
            "146", "147", "148", "149", "150",
        },
    };

    auto result = splitListIntoBatches(input);

    EXPECT_EQ(result, expectation);
}

TEST(Helpers, NotEnoughForMoreThanOneBatch)
{
    QStringList input{
        "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10", "11", "12",
        "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24",
        "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36",
        "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48",
        "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
        "61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72",
        "73", "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "84",
        "85", "86", "87", "88", "89", "90",
    };

    std::vector<QStringList> expectation = {
        {
            "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10",
            "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
            "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
            "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
            "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
            "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
            "61", "62", "63", "64", "65", "66", "67", "68", "69", "70",
            "71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
            "81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
        },
    };

    auto result = splitListIntoBatches(input);

    EXPECT_EQ(result, expectation);
}

TEST(Helpers, BatchThreeParts)
{
    QStringList input{
        "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "10",
        "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",  "20",
        "21",  "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",  "30",
        "31",  "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",  "40",
        "41",  "42",  "43",  "44",  "45",  "46",  "47",  "48",  "49",  "50",
        "51",  "52",  "53",  "54",  "55",  "56",  "57",  "58",  "59",  "60",
        "61",  "62",  "63",  "64",  "65",  "66",  "67",  "68",  "69",  "70",
        "71",  "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",  "80",
        "81",  "82",  "83",  "84",  "85",  "86",  "87",  "88",  "89",  "90",
        "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",  "99",  "100",
        "101", "102", "103", "104", "105", "106", "107", "108", "109", "110",
        "111", "112", "113", "114", "115", "116", "117", "118", "119", "120",
        "121", "122", "123", "124", "125", "126", "127", "128", "129", "130",
        "131", "132", "133", "134", "135", "136", "137", "138", "139", "140",
        "141", "142", "143", "144", "145", "146", "147", "148", "149", "150",
        "151", "152", "153", "154", "155", "156", "157", "158", "159", "160",
        "161", "162", "163", "164", "165", "166", "167", "168", "169", "170",
        "171", "172", "173", "174", "175", "176", "177", "178", "179", "180",
        "181", "182", "183", "184", "185", "186", "187", "188", "189", "190",
        "191", "192", "193", "194", "195", "196", "197", "198", "199", "200",
        "201", "202", "203", "204", "205", "206", "207", "208", "209", "210",
        "211", "212", "213", "214", "215", "216", "217", "218", "219", "220",
        "221", "222", "223", "224", "225", "226", "227", "228", "229", "230",
        "231", "232", "233", "234", "235", "236", "237", "238", "239", "240",
        "241", "242", "243", "244", "245", "246", "247", "248", "249", "250",
    };

    std::vector<QStringList> expectation = {
        {
            "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10",
            "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
            "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
            "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
            "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
            "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
            "61", "62", "63", "64", "65", "66", "67", "68", "69", "70",
            "71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
            "81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
            "91", "92", "93", "94", "95", "96", "97", "98", "99", "100",
        },
        {
            "101", "102", "103", "104", "105", "106", "107", "108", "109",
            "110", "111", "112", "113", "114", "115", "116", "117", "118",
            "119", "120", "121", "122", "123", "124", "125", "126", "127",
            "128", "129", "130", "131", "132", "133", "134", "135", "136",
            "137", "138", "139", "140", "141", "142", "143", "144", "145",
            "146", "147", "148", "149", "150", "151", "152", "153", "154",
            "155", "156", "157", "158", "159", "160", "161", "162", "163",
            "164", "165", "166", "167", "168", "169", "170", "171", "172",
            "173", "174", "175", "176", "177", "178", "179", "180", "181",
            "182", "183", "184", "185", "186", "187", "188", "189", "190",
            "191", "192", "193", "194", "195", "196", "197", "198", "199",
            "200",
        },
        {
            "201", "202", "203", "204", "205", "206", "207", "208", "209",
            "210", "211", "212", "213", "214", "215", "216", "217", "218",
            "219", "220", "221", "222", "223", "224", "225", "226", "227",
            "228", "229", "230", "231", "232", "233", "234", "235", "236",
            "237", "238", "239", "240", "241", "242", "243", "244", "245",
            "246", "247", "248", "249", "250",
        },
    };

    auto result = splitListIntoBatches(input);

    EXPECT_EQ(result, expectation);
}

TEST(Helpers, BatchOnePartDifferentSize)
{
    QStringList input{
        "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "10", "11", "12",
        "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23",
    };

    std::vector<QStringList> expectation{
        {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
        },
        {
            "11",
            "12",
            "13",
            "14",
            "15",
            "16",
            "17",
            "18",
            "19",
            "20",
        },
        {
            "21",
            "22",
            "23",
        },
    };

    auto result = splitListIntoBatches(input, 10);

    EXPECT_EQ(result, expectation);
}

TEST(Helpers, BatchDifferentInputType)
{
    QList<int> input{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    };

    std::vector<QList<int>> expectation{
        {
            1,
            2,
            3,
            4,
            5,
            6,
        },
        {
            7,
            8,
            9,
            10,
            11,
            12,
        },
        {
            13,
            14,
        },
    };

    auto result = splitListIntoBatches(input, 6);

    EXPECT_EQ(result, expectation);
}

TEST(Helpers, skipSpace)
{
    struct TestCase {
        QString input;
        SizeType startIdx;
        SizeType expected;
    };

    std::vector<TestCase> tests{{"foo    bar", 3, 6}, {"foo bar", 3, 3},
                                {"foo ", 3, 3},       {"foo    ", 3, 6},
                                {"   ", 0, 2},        {" ", 0, 0}};

    for (const auto &c : tests)
    {
        const auto actual = skipSpace(c.input, c.startIdx);

        EXPECT_EQ(actual, c.expected)
            << actual << " (" << c.input << ") did not match expected value "
            << c.expected;
    }
}

TEST(Helpers, findUnitMultiplierToSec)
{
    constexpr uint64_t sec = 1;
    constexpr uint64_t min = 60;
    constexpr uint64_t hour = min * 60;
    constexpr uint64_t day = hour * 24;
    constexpr uint64_t week = day * 7;
    constexpr uint64_t month = day * 30;
    constexpr uint64_t bad = 0;

    struct TestCase {
        QString input;
        SizeType startPos;
        SizeType expectedEndPos;
        uint64_t expectedMultiplier;
    };

    std::vector<TestCase> tests{
        {"s", 0, 0, sec},
        {"m", 0, 0, min},
        {"h", 0, 0, hour},
        {"d", 0, 0, day},
        {"w", 0, 0, week},
        {"mo", 0, 1, month},

        {"s alienpls", 0, 0, sec},
        {"m alienpls", 0, 0, min},
        {"h alienpls", 0, 0, hour},
        {"d alienpls", 0, 0, day},
        {"w alienpls", 0, 0, week},
        {"mo alienpls", 0, 1, month},

        {"alienpls s", 9, 9, sec},
        {"alienpls m", 9, 9, min},
        {"alienpls h", 9, 9, hour},
        {"alienpls d", 9, 9, day},
        {"alienpls w", 9, 9, week},
        {"alienpls mo", 9, 10, month},

        {"alienpls s alienpls", 9, 9, sec},
        {"alienpls m alienpls", 9, 9, min},
        {"alienpls h alienpls", 9, 9, hour},
        {"alienpls d alienpls", 9, 9, day},
        {"alienpls w alienpls", 9, 9, week},
        {"alienpls mo alienpls", 9, 10, month},

        {"second", 0, 5, sec},
        {"minute", 0, 5, min},
        {"hour", 0, 3, hour},
        {"day", 0, 2, day},
        {"week", 0, 3, week},
        {"month", 0, 4, month},

        {"alienpls2 second", 10, 15, sec},
        {"alienpls2 minute", 10, 15, min},
        {"alienpls2 hour", 10, 13, hour},
        {"alienpls2 day", 10, 12, day},
        {"alienpls2 week", 10, 13, week},
        {"alienpls2 month", 10, 14, month},

        {"alienpls2 second alienpls", 10, 15, sec},
        {"alienpls2 minute alienpls", 10, 15, min},
        {"alienpls2 hour alienpls", 10, 13, hour},
        {"alienpls2 day alienpls", 10, 12, day},
        {"alienpls2 week alienpls", 10, 13, week},
        {"alienpls2 month alienpls", 10, 14, month},

        {"seconds", 0, 6, sec},
        {"minutes", 0, 6, min},
        {"hours", 0, 4, hour},
        {"days", 0, 3, day},
        {"weeks", 0, 4, week},
        {"months", 0, 5, month},

        {"alienpls2 seconds", 10, 16, sec},
        {"alienpls2 minutes", 10, 16, min},
        {"alienpls2 hours", 10, 14, hour},
        {"alienpls2 days", 10, 13, day},
        {"alienpls2 weeks", 10, 14, week},
        {"alienpls2 months", 10, 15, month},

        {"alienpls2 seconds alienpls", 10, 16, sec},
        {"alienpls2 minutes alienpls", 10, 16, min},
        {"alienpls2 hours alienpls", 10, 14, hour},
        {"alienpls2 days alienpls", 10, 13, day},
        {"alienpls2 weeks alienpls", 10, 14, week},
        {"alienpls2 months alienpls", 10, 15, month},

        {"sec", 0, 0, bad},
        {"min", 0, 0, bad},
        {"ho", 0, 0, bad},
        {"da", 0, 0, bad},
        {"we", 0, 0, bad},
        {"mon", 0, 0, bad},
        {"foo", 0, 0, bad},
        {"S", 0, 0, bad},
        {"M", 0, 0, bad},
        {"H", 0, 0, bad},
        {"D", 0, 0, bad},
        {"W", 0, 0, bad},
        {"MO", 0, 1, bad},

        {"alienpls2 sec", 10, 0, bad},
        {"alienpls2 min", 10, 0, bad},
        {"alienpls2 ho", 10, 0, bad},
        {"alienpls2 da", 10, 0, bad},
        {"alienpls2 we", 10, 0, bad},
        {"alienpls2 mon", 10, 0, bad},
        {"alienpls2 foo", 10, 0, bad},
        {"alienpls2 S", 10, 0, bad},
        {"alienpls2 M", 10, 0, bad},
        {"alienpls2 H", 10, 0, bad},
        {"alienpls2 D", 10, 0, bad},
        {"alienpls2 W", 10, 0, bad},
        {"alienpls2 MO", 10, 0, bad},

        {"alienpls2 sec alienpls", 10, 0, bad},
        {"alienpls2 min alienpls", 10, 0, bad},
        {"alienpls2 ho alienpls", 10, 0, bad},
        {"alienpls2 da alienpls", 10, 0, bad},
        {"alienpls2 we alienpls", 10, 0, bad},
        {"alienpls2 mon alienpls", 10, 0, bad},
        {"alienpls2 foo alienpls", 10, 0, bad},
        {"alienpls2 S alienpls", 10, 0, bad},
        {"alienpls2 M alienpls", 10, 0, bad},
        {"alienpls2 H alienpls", 10, 0, bad},
        {"alienpls2 D alienpls", 10, 0, bad},
        {"alienpls2 W alienpls", 10, 0, bad},
        {"alienpls2 MO alienpls", 10, 0, bad},
    };

    for (const auto &c : tests)
    {
        SizeType pos = c.startPos;
        const auto actual = findUnitMultiplierToSec(c.input, pos);

        if (c.expectedMultiplier == bad)
        {
            EXPECT_FALSE(actual.second) << c.input;
        }
        else
        {
            EXPECT_TRUE(pos == c.expectedEndPos && actual.second &&
                        actual.first == c.expectedMultiplier)
                << c.input << ": Expected(end: " << c.expectedEndPos
                << ", mult: " << c.expectedMultiplier << ") Actual(end: " << pos
                << ", mult: " << actual.first << ")";
        }
    }
}

TEST(Helpers, parseDurationToSeconds)
{
    struct TestCase {
        QString input;
        int64_t output;
        int64_t noUnitMultiplier = 1;
    };

    auto wrongInput = [](QString &&input) {
        return TestCase{input, -1};
    };

    std::vector<TestCase> tests{
        {"1 minutes 9s", 69},
        {"22 m  17 s", 1337},
        {"7d 5h 10m 52s", 623452},
        {"2h 19m 5s  ", 8345},
        {"3d 15 h 13m 54s", 314034},
        {"27s", 27},
        {
            "9h 36 m 29s", 34589,
            7,  // should be unused
        },
        {"1h 59s", 3659},
        {"12d2h22m25s", 1045345},
        {"2h22m25s12d", 1045345},
        {"1d32s", 86432},
        {"0", 0},
        {"0 s", 0},
        {"1weeks", 604800},
        {"2 day5days", 604800},
        {"1 day", 86400},
        {"4 hours 30m 19h 30 minute", 86400},
        {"3 months", 7776000},
        {"1 mo 2month", 7776000},
        // from documentation
        {"1w 2h", 612000},
        {"1w 1w 0s 4d", 1555200},
        {"5s3h4w", 2430005},
        // from twitch response
        {"30m", 1800},
        {"1 week", 604800},
        {"5 days 12 hours", 475200},
        // noUnitMultiplier
        {"0", 0, 60},
        {
            "60", 3600,
            60,  // minute
        },
        {
            "1",
            86400,  // 1d
            86400,
        },
        // wrong input
        wrongInput("1min"),
        wrongInput(""),
        wrongInput("1m5w+5"),
        wrongInput("1h30"),
        wrongInput("12 34w"),
        wrongInput("4W"),
        wrongInput("1min"),
        wrongInput("4Min"),
        wrongInput("4sec"),
    };

    for (const auto &c : tests)
    {
        const auto actual = parseDurationToSeconds(c.input, c.noUnitMultiplier);

        EXPECT_EQ(actual, c.output)
            << actual << " (" << c.input << ") did not match expected value "
            << c.output;
    }
}

TEST(Helpers, unescapeZeroWidthJoiner)
{
    struct TestCase {
        QStringView input;
        QStringView output;
    };

    std::vector<TestCase> tests{
        {u"foo bar", u"foo bar"},
        {u"", u""},
        {u"a", u"a"},
        {u"\U000E0002", u"\u200D"},
        {u"foo\U000E0002bar", u"foo\u200Dbar"},
        {u"foo \U000E0002 bar", u"foo \u200D bar"},
        {u"\U0001F468\U000E0002\U0001F33E", u"\U0001F468\u200D\U0001F33E"},
        // don't replace ZWJ
        {u"\U0001F468\u200D\U0001F33E", u"\U0001F468\u200D\U0001F33E"},
        // only replace the first escape tag in sequences
        {
            u"\U0001F468\U000E0002\U000E0002\U0001F33E",
            u"\U0001F468\u200D\U000E0002\U0001F33E",
        },
        {
            u"\U0001F468\U000E0002\U000E0002\U000E0002\U0001F33E",
            u"\U0001F468\u200D\U000E0002\U000E0002\U0001F33E",
        },
    };

    // sanity check that the compiler supports unicode string literals
    static_assert(
        [] {
            constexpr std::span zwj = u"\u200D";
            static_assert(zwj.size() == 2);
            static_assert(zwj[0] == u'\x200D');
            static_assert(zwj[1] == u'\0');

            constexpr std::span escapeTag = u"\U000E0002";
            static_assert(escapeTag.size() == 3);
            static_assert(escapeTag[0] == u'\xDB40');
            static_assert(escapeTag[1] == u'\xDC02');
            static_assert(escapeTag[2] == u'\0');

            return true;
        }(),
        "The compiler must support Unicode string literals");

    for (const auto &c : tests)
    {
        const auto actual = unescapeZeroWidthJoiner(c.input.toString());

        EXPECT_EQ(actual, c.output);
    }
}

TEST(Helpers, chronoToQDateTime)
{
    mock::BaseApplication app;

    auto epoch = chronoToQDateTime({});
    ASSERT_EQ(epoch.timeZone(), QTimeZone::utc());
    ASSERT_EQ(epoch.toMSecsSinceEpoch(), 0);

    std::chrono::milliseconds somePointSinceEpoch{1740574189131};
    auto qPointSinceEpoch = chronoToQDateTime(
        std::chrono::system_clock::time_point{somePointSinceEpoch});
    ASSERT_EQ(qPointSinceEpoch.timeZone(), QTimeZone::utc());
    ASSERT_EQ(qPointSinceEpoch.toMSecsSinceEpoch(),
              somePointSinceEpoch.count());
    ASSERT_EQ(qPointSinceEpoch.toString(Qt::ISODateWithMs),
              "2025-02-26T12:49:49.131Z");
}

TEST(Helpers, codepointSlice)
{
    ASSERT_EQ(codepointSlice(u"", 0, 0), u"");
    ASSERT_EQ(codepointSlice(u"", 0, 1), u"");
    ASSERT_EQ(codepointSlice(u"", 1, 1), u"");
    ASSERT_EQ(codepointSlice(u"", -1, 1), u"");

    ASSERT_EQ(codepointSlice(u"a", 0, 0), u"");
    ASSERT_EQ(codepointSlice(u"a", 0, 1), u"a");
    ASSERT_EQ(codepointSlice(u"a", 0, 2), u"");
    ASSERT_EQ(codepointSlice(u"a", -1, 1), u"");

    ASSERT_EQ(codepointSlice(u"abcd", 1, 3), u"bc");
    ASSERT_EQ(codepointSlice(u"abcd", 0, 3), u"abc");
    ASSERT_EQ(codepointSlice(u"abcd", 1, 4), u"bcd");
    ASSERT_EQ(codepointSlice(u"abcd", 0, 4), u"abcd");
    ASSERT_EQ(codepointSlice(u"abcd", 0, 5), u"");
    ASSERT_EQ(codepointSlice(u"abcd", 5, 0), u"");

    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 1, 3), u"🍟🥚");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 0, 3), u"🍩🍟🥚");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 0, 9),
              u"🍩🍟🥚🍳🌮🍞🌭🥞🍳");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 3, 9), u"🍳🌮🍞🌭🥞🍳");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 3, 10), u"");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 3, 8), u"🍳🌮🍞🌭🥞");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 3, 7), u"🍳🌮🍞🌭");
    ASSERT_EQ(codepointSlice(u"🍩🍟🥚🍳🌮🍞🌭🥞🍳", 3, 4), u"🍳");

    ASSERT_EQ(codepointSlice(u"🍩🍟\xD83E\xDD5A", 0, 3), u"🍩🍟🥚");
    ASSERT_EQ(codepointSlice(u"🍩🍟\xD83E\xDD5A", 0, 4), u"");
    ASSERT_EQ(codepointSlice(u"🍩🍟\xD83E", 0, 3), u"🍩🍟\xD83E");
    ASSERT_EQ(codepointSlice(u"🍩🍟\xD83E🥚", 0, 4), u"🍩🍟\xD83E🥚");
}
