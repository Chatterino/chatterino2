#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>
#include <QString>

#include <string>

namespace chatterino::eventsub::lib {

boost::json::result_for<String, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<String> /*tag*/,
    const boost::json::value &jvRoot)
{
    const auto v = boost::json::try_value_to<std::string>(jvRoot);
    if (v.has_error())
    {
        return v.error();
    }

    return String(v.value());
}

}  // namespace chatterino::eventsub::lib
