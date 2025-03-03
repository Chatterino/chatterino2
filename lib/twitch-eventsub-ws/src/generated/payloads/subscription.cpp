// WARNING: This file is automatically generated. Any changes will be lost.
#include "twitch-eventsub-ws/chrono.hpp"  // IWYU pragma: keep
#include "twitch-eventsub-ws/detail/errors.hpp"
#include "twitch-eventsub-ws/detail/variant.hpp"  // IWYU pragma: keep
#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::subscription {

boost::json::result_for<Transport, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Transport> /* tag */,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        EVENTSUB_BAIL_HERE(error::Kind::ExpectedObject);
    }
    const auto &root = jvRoot.get_object();

    const auto *jvmethod = root.if_contains("method");
    if (jvmethod == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto method = boost::json::try_value_to<std::string>(*jvmethod);

    if (method.has_error())
    {
        return method.error();
    }

    const auto *jvsessionID = root.if_contains("session_id");
    if (jvsessionID == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
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
    boost::json::try_value_to_tag<Subscription> /* tag */,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        EVENTSUB_BAIL_HERE(error::Kind::ExpectedObject);
    }
    const auto &root = jvRoot.get_object();

    const auto *jvid = root.if_contains("id");
    if (jvid == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto id = boost::json::try_value_to<std::string>(*jvid);

    if (id.has_error())
    {
        return id.error();
    }

    const auto *jvstatus = root.if_contains("status");
    if (jvstatus == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto status = boost::json::try_value_to<std::string>(*jvstatus);

    if (status.has_error())
    {
        return status.error();
    }

    const auto *jvtype = root.if_contains("type");
    if (jvtype == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto type = boost::json::try_value_to<std::string>(*jvtype);

    if (type.has_error())
    {
        return type.error();
    }

    const auto *jvversion = root.if_contains("version");
    if (jvversion == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto version = boost::json::try_value_to<std::string>(*jvversion);

    if (version.has_error())
    {
        return version.error();
    }

    const auto *jvtransport = root.if_contains("transport");
    if (jvtransport == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto transport = boost::json::try_value_to<Transport>(*jvtransport);

    if (transport.has_error())
    {
        return transport.error();
    }

    const auto *jvcreatedAt = root.if_contains("created_at");
    if (jvcreatedAt == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
    }

    auto createdAt = boost::json::try_value_to<std::string>(*jvcreatedAt);

    if (createdAt.has_error())
    {
        return createdAt.error();
    }

    static_assert(std::is_trivially_copyable_v<std::remove_reference_t<
                      decltype(std::declval<Subscription>().cost)>>);
    const auto *jvcost = root.if_contains("cost");
    if (jvcost == nullptr)
    {
        EVENTSUB_BAIL_HERE(error::Kind::FieldMissing);
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
        .cost = cost.value(),
    };
}

}  // namespace chatterino::eventsub::lib::payload::subscription
