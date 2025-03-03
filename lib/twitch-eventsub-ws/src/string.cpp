#include "twitch-eventsub-ws/string.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>
#include <QString>

namespace chatterino::eventsub::lib {

boost::json::result_for<String, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<String> /*tag*/,
    const boost::json::value &jvRoot)
{
    if (jvRoot.is_null())
    {
        // We treat null "strings" as empty strings
        return String();
    }

    if (!jvRoot.is_string())
    {
        // we don't need a source_location - it doesn't tell us much
        return lib::error::makeCode(error::Kind::ExpectedString, nullptr);
    }

    const auto &str = jvRoot.get_string();
    return String(str);
}

}  // namespace chatterino::eventsub::lib
