#include "twitch-eventsub-ws/chrono.hpp"

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
    tag_invoke(
        boost::json::try_value_to_tag<std::chrono::system_clock::time_point>,
        const boost::json::value &jvRoot, const AsISO8601 &)
{
    const auto raw = boost::json::try_value_to<std::string>(jvRoot);
    if (raw.has_error())
    {
        return raw.error();
    }

    std::chrono::system_clock::time_point tp;
    std::istringstream in{*raw};
    in >> parse("%FT%H:%M:%12SZ", tp);

    return tp;
}

}  // namespace chatterino::eventsub::lib
