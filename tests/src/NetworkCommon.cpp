#include "common/network/NetworkCommon.hpp"

#include "Test.hpp"

using namespace chatterino;

TEST(NetworkCommon, parseHeaderList1)
{
    const QString input = "Authorization:secretKey;NextHeader:boo";
    const std::vector<std::pair<QByteArray, QByteArray>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListTrimmed)
{
    const QString input = "Authorization:  secretKey; NextHeader   :boo";
    const std::vector<std::pair<QByteArray, QByteArray>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListColonInValue)
{
    // The input values first header pair contains an invalid value, too many colons. We expect this value to be skipped
    const QString input = "Authorization:  secretKey:hehe; NextHeader   :boo";
    const std::vector<std::pair<QByteArray, QByteArray>> expected = {
        {"Authorization", "secretKey:hehe"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListBadPair)
{
    // The input values first header pair doesn't have a colon, so we don't know where the header name and value start/end
    const QString input = "Authorization  secretKeybad; NextHeader   :boo";
    const std::vector<std::pair<QByteArray, QByteArray>> expected = {
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
    ASSERT_EQ(1, actual.size());
}
