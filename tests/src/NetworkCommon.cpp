#include "common/NetworkCommon.hpp"

using namespace chatterino;

TEST(NetworkCommon, parseHeaderList1)
{
    const QString input = "Authorization:secretkey;NextHeader:boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}

TEST(NetworkCommon, parseHeaderListTrimmed)
{
    const QString input = "Authorization:  secretkey; NextHeader   :boo";
    const std::vector<std::pair<QString, QString>> expected = {
        {"Authorization", "secretKey"},
        {"NextHeader", "boo"},
    };

    const auto actual = parseHeaderList(input);

    ASSERT_EQ(expected, actual);
}
