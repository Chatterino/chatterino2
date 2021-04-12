#include "common/NetworkCommon.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

TEST(NetworkCommon, parseHeaderList1)
{
    const QString input = "Authorization:secretKey;NextHeader:boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListTrimmed)
{
    const QString input = "Authorization:  secretKey; NextHeader   :boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListBadPair)
{
    // The input values first header pair contains an invalid value, too many colons. We expect this value to be skipped
    const QString input = "Authorization:  secretKey:bad; NextHeader   :boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
    ASSERT_EQ(1, actual.size());
}

TEST(NetworkCommon, parseHeaderListBadPair2)
{
    // The input values first header pair doesn't have a colon, so we don't know where the header name and value start/end
    const QString input = "Authorization  secretKeybad; NextHeader   :boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
    ASSERT_EQ(1, actual.size());
}
