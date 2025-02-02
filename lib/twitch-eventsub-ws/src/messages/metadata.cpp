#include "twitch-eventsub-ws/messages/metadata.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::messages {

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

    auto messageID = boost::json::try_value_to<std::string>(*jvmessageID);

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

    auto messageType = boost::json::try_value_to<std::string>(*jvmessageType);

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

    auto messageTimestamp =
        boost::json::try_value_to<std::string>(*jvmessageTimestamp);

    if (messageTimestamp.has_error())
    {
        return messageTimestamp.error();
    }

    std::optional<std::string> subscriptionType = std::nullopt;
    const auto *jvsubscriptionType = root.if_contains("subscription_type");
    if (jvsubscriptionType != nullptr && !jvsubscriptionType->is_null())
    {
        auto tsubscriptionType =
            boost::json::try_value_to<std::string>(*jvsubscriptionType);

        if (tsubscriptionType.has_error())
        {
            return tsubscriptionType.error();
        }
        subscriptionType = std::move(tsubscriptionType.value());
    }

    std::optional<std::string> subscriptionVersion = std::nullopt;
    const auto *jvsubscriptionVersion =
        root.if_contains("subscription_version");
    if (jvsubscriptionVersion != nullptr && !jvsubscriptionVersion->is_null())
    {
        auto tsubscriptionVersion =
            boost::json::try_value_to<std::string>(*jvsubscriptionVersion);

        if (tsubscriptionVersion.has_error())
        {
            return tsubscriptionVersion.error();
        }
        subscriptionVersion = std::move(tsubscriptionVersion.value());
    }

    return Metadata{
        .messageID = std::move(messageID.value()),
        .messageType = std::move(messageType.value()),
        .messageTimestamp = std::move(messageTimestamp.value()),
        .subscriptionType = std::move(subscriptionType),
        .subscriptionVersion = std::move(subscriptionVersion),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::messages
