#include "twitch-eventsub-ws/errors.hpp"

#include <gtest/gtest.h>
#include <twitch-eventsub-ws/chrono.hpp>

using namespace chatterino::eventsub;

namespace {

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr auto MAY14 =
    std::chrono::sys_days{May / 14 / 2024} + 12h + 31min + 47s;
constexpr auto MAY14_PRECISE = MAY14 + 123ms + 456us;

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    buildTP(std::string_view sv)
{
    auto xd = boost::json::string(sv);

    return boost::json::try_value_to<std::chrono::system_clock::time_point>(
        xd, lib::AsISO8601());
}

}  // namespace

TEST(Chrono, Success)
{
    {
        auto oTp = buildTP("2024-05-14T12:31:47Z");
        ASSERT_FALSE(oTp.has_error());
        ASSERT_EQ(oTp.value(), MAY14);
    }
    {
        std::chrono::system_clock::time_point tp;
        auto oTp = buildTP("2024-05-14T12:31:47.123456789Z");
        ASSERT_FALSE(oTp.has_error());
        ASSERT_EQ(std::chrono::time_point_cast<std::chrono::microseconds>(
                      oTp.value()),
                  MAY14_PRECISE);
    }
}

TEST(Chrono, Fail)
{
    {
        auto oTp = buildTP("2024-05-14T12:31:47.123456789123456789Z");
        ASSERT_TRUE(oTp.has_error());
        ASSERT_EQ(static_cast<lib::error::Kind>(oTp.error().value()),
                  lib::error::Kind::BadTimeFormat);
    }
}
