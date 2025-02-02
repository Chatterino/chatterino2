#include "twitch-eventsub-ws/payloads/session-welcome.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::session_welcome {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Payload must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &outerRoot = jvRoot.get_object();

    const auto *jvInnerRoot = outerRoot.if_contains("session");
    if (jvInnerRoot == nullptr)
    {
        static const error::ApplicationErrorCategory errorMissing{
            "Payload's key session is missing"};
        return boost::system::error_code{129, errorMissing};
    }
    if (!jvInnerRoot->is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Payload's session must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvInnerRoot->get_object();

    const auto *jvid = root.if_contains("id");
    if (jvid == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_id{
            "Missing required key id"};
        return boost::system::error_code{129, error_missing_field_id};
    }

    auto id = boost::json::try_value_to<std::string>(*jvid);

    if (id.has_error())
    {
        return id.error();
    }

    return Payload{
        .id = std::move(id.value()),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::payload::session_welcome
