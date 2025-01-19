#include "twitch-eventsub-ws/chrono.hpp"

namespace eventsub {

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

    std::istringstream in{*raw};
    std::chrono::system_clock::time_point tp;
    in >> date::parse("%FT%TZ", tp);

    return tp;
}
}  // namespace eventsub
