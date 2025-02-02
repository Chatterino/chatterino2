#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::subscription {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Transport, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Transport>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Transport must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvmethod = root.if_contains("method");
    if (jvmethod == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_method{
            "Missing required key method"};
        return boost::system::error_code{129, error_missing_field_method};
    }

    auto method = boost::json::try_value_to<std::string>(*jvmethod);

    if (method.has_error())
    {
        return method.error();
    }

    const auto *jvsessionID = root.if_contains("session_id");
    if (jvsessionID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_sessionID{"Missing required key session_id"};
        return boost::system::error_code{129, error_missing_field_sessionID};
    }

    auto sessionID = boost::json::try_value_to<std::string>(*jvsessionID);

    if (sessionID.has_error())
    {
        return sessionID.error();
    }

    return Transport{
        .method = std::move(method.value()),
        .sessionID = std::move(sessionID.value()),
    };
}

boost::json::result_for<Subscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subscription>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Subscription must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

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

    const auto *jvstatus = root.if_contains("status");
    if (jvstatus == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_status{
            "Missing required key status"};
        return boost::system::error_code{129, error_missing_field_status};
    }

    auto status = boost::json::try_value_to<std::string>(*jvstatus);

    if (status.has_error())
    {
        return status.error();
    }

    const auto *jvtype = root.if_contains("type");
    if (jvtype == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_type{
            "Missing required key type"};
        return boost::system::error_code{129, error_missing_field_type};
    }

    auto type = boost::json::try_value_to<std::string>(*jvtype);

    if (type.has_error())
    {
        return type.error();
    }

    const auto *jvversion = root.if_contains("version");
    if (jvversion == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_version{"Missing required key version"};
        return boost::system::error_code{129, error_missing_field_version};
    }

    auto version = boost::json::try_value_to<std::string>(*jvversion);

    if (version.has_error())
    {
        return version.error();
    }

    const auto *jvtransport = root.if_contains("transport");
    if (jvtransport == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_transport{"Missing required key transport"};
        return boost::system::error_code{129, error_missing_field_transport};
    }

    auto transport = boost::json::try_value_to<Transport>(*jvtransport);

    if (transport.has_error())
    {
        return transport.error();
    }

    const auto *jvcreatedAt = root.if_contains("created_at");
    if (jvcreatedAt == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_createdAt{"Missing required key created_at"};
        return boost::system::error_code{129, error_missing_field_createdAt};
    }

    auto createdAt = boost::json::try_value_to<std::string>(*jvcreatedAt);

    if (createdAt.has_error())
    {
        return createdAt.error();
    }

    const auto *jvcost = root.if_contains("cost");
    if (jvcost == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_cost{
            "Missing required key cost"};
        return boost::system::error_code{129, error_missing_field_cost};
    }

    auto cost = boost::json::try_value_to<int>(*jvcost);

    if (cost.has_error())
    {
        return cost.error();
    }

    return Subscription{
        .id = std::move(id.value()),
        .status = std::move(status.value()),
        .type = std::move(type.value()),
        .version = std::move(version.value()),
        .transport = std::move(transport.value()),
        .createdAt = std::move(createdAt.value()),
        .cost = std::move(cost.value()),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::payload::subscription
