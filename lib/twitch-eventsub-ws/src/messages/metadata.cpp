#include "twitch-eventsub-ws/messages/metadata.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::messages {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Metadata, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Metadata>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Metadata must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvmessageID = root.if_contains("message_id");
    if (jvmessageID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_messageID{"Missing required key message_id"};
        return boost::system::error_code{129, error_missing_field_messageID};
    }

    const auto messageID = boost::json::try_value_to<std::string>(*jvmessageID);

    if (messageID.has_error())
    {
        return messageID.error();
    }

    const auto *jvmessageType = root.if_contains("message_type");
    if (jvmessageType == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_messageType{
                "Missing required key message_type"};
        return boost::system::error_code{129, error_missing_field_messageType};
    }

    const auto messageType =
        boost::json::try_value_to<std::string>(*jvmessageType);

    if (messageType.has_error())
    {
        return messageType.error();
    }

    const auto *jvmessageTimestamp = root.if_contains("message_timestamp");
    if (jvmessageTimestamp == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_messageTimestamp{
                "Missing required key message_timestamp"};
        return boost::system::error_code{129,
                                         error_missing_field_messageTimestamp};
    }

    const auto messageTimestamp =
        boost::json::try_value_to<std::string>(*jvmessageTimestamp);

    if (messageTimestamp.has_error())
    {
        return messageTimestamp.error();
    }

    std::optional<std::string> subscriptionType = std::nullopt;
    const auto *jvsubscriptionType = root.if_contains("subscription_type");
    if (jvsubscriptionType != nullptr && !jvsubscriptionType->is_null())
    {
        const auto tsubscriptionType =
            boost::json::try_value_to<std::string>(*jvsubscriptionType);

        if (tsubscriptionType.has_error())
        {
            return tsubscriptionType.error();
        }
        subscriptionType = tsubscriptionType.value();
    }

    std::optional<std::string> subscriptionVersion = std::nullopt;
    const auto *jvsubscriptionVersion =
        root.if_contains("subscription_version");
    if (jvsubscriptionVersion != nullptr && !jvsubscriptionVersion->is_null())
    {
        const auto tsubscriptionVersion =
            boost::json::try_value_to<std::string>(*jvsubscriptionVersion);

        if (tsubscriptionVersion.has_error())
        {
            return tsubscriptionVersion.error();
        }
        subscriptionVersion = tsubscriptionVersion.value();
    }

    return Metadata{
        .messageID = messageID.value(),
        .messageType = messageType.value(),
        .messageTimestamp = messageTimestamp.value(),
        .subscriptionType = subscriptionType,
        .subscriptionVersion = subscriptionVersion,
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace eventsub::messages
