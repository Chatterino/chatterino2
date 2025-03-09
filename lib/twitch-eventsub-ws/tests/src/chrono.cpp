#include <gtest/gtest.h>
#include <twitch-eventsub-ws/chrono.hpp>

namespace {

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr auto MAY14 =
    std::chrono::sys_days{May / 14 / 2024} + 12h + 31min + 47s;

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    buildTP(std::string_view sv)
{
    auto xd = boost::json::string(sv);

    return boost::json::try_value_to<std::chrono::system_clock::time_point>(
        xd, chatterino::eventsub::lib::AsISO8601());
}

}  // namespace

TEST(Chrono, Construction)
{
    {
        auto oTp = buildTP("2024-05-14T12:31:47Z");
        ASSERT_FALSE(oTp.has_error());
        ASSERT_EQ(oTp.value(), MAY14);
    }
}
