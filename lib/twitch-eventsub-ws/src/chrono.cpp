#include "twitch-eventsub-ws/chrono.hpp"

#include "twitch-eventsub-ws/date.h"

#include <sstream>

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
    in >> date::parse("%FT%H:%M:%12SZ", tp);

    return tp;
}
}  // namespace chatterino::eventsub::lib
