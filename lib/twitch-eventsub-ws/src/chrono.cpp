#include "twitch-eventsub-ws/chrono.hpp"

#include "twitch-eventsub-ws/detail/errors.hpp"

#if __cpp_lib_chrono >= 201907L
#    include <chrono>
using namespace std::chrono;
#else
#    include <date/date.h>
using namespace date;
#endif

namespace chatterino::eventsub::lib {

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<
                   std::chrono::system_clock::time_point> /*tag*/,
               const boost::json::value &jvRoot, const AsISO8601 & /*tag*/)
{
    const auto raw = boost::json::try_value_to<std::string>(jvRoot);
    if (raw.has_error())
    {
        return raw.error();
    }

    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>
        tp;
    std::istringstream in{*raw};
    in >> parse("%FT%H:%M:%12SZ", tp);

    if (!in.good())
    {
        EVENTSUB_BAIL_HERE(error::Kind::BadTimeFormat);
    }

    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        tp);
}

}  // namespace chatterino::eventsub::lib
