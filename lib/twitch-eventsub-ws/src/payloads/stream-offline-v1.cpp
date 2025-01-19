#include "twitch-eventsub-ws/payloads/stream-offline-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::payload::stream_offline::v1 {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Event must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvbroadcasterUserID = root.if_contains("broadcaster_user_id");
    if (jvbroadcasterUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_broadcasterUserID{
                "Missing required key broadcaster_user_id"};
        return boost::system::error_code{129,
                                         error_missing_field_broadcasterUserID};
    }

    const auto broadcasterUserID =
        boost::json::try_value_to<std::string>(*jvbroadcasterUserID);

    if (broadcasterUserID.has_error())
    {
        return broadcasterUserID.error();
    }

    const auto *jvbroadcasterUserLogin =
        root.if_contains("broadcaster_user_login");
    if (jvbroadcasterUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_broadcasterUserLogin{
                "Missing required key broadcaster_user_login"};
        return boost::system::error_code{
            129, error_missing_field_broadcasterUserLogin};
    }

    const auto broadcasterUserLogin =
        boost::json::try_value_to<std::string>(*jvbroadcasterUserLogin);

    if (broadcasterUserLogin.has_error())
    {
        return broadcasterUserLogin.error();
    }

    const auto *jvbroadcasterUserName =
        root.if_contains("broadcaster_user_name");
    if (jvbroadcasterUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_broadcasterUserName{
                "Missing required key broadcaster_user_name"};
        return boost::system::error_code{
            129, error_missing_field_broadcasterUserName};
    }

    const auto broadcasterUserName =
        boost::json::try_value_to<std::string>(*jvbroadcasterUserName);

    if (broadcasterUserName.has_error())
    {
        return broadcasterUserName.error();
    }

    return Event{
        .broadcasterUserID = broadcasterUserID.value(),
        .broadcasterUserLogin = broadcasterUserLogin.value(),
        .broadcasterUserName = broadcasterUserName.value(),
    };
}

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Payload must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvsubscription = root.if_contains("subscription");
    if (jvsubscription == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subscription{
                "Missing required key subscription"};
        return boost::system::error_code{129, error_missing_field_subscription};
    }

    const auto subscription =
        boost::json::try_value_to<subscription::Subscription>(*jvsubscription);

    if (subscription.has_error())
    {
        return subscription.error();
    }

    const auto *jvevent = root.if_contains("event");
    if (jvevent == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_event{
            "Missing required key event"};
        return boost::system::error_code{129, error_missing_field_event};
    }

    const auto event = boost::json::try_value_to<Event>(*jvevent);

    if (event.has_error())
    {
        return event.error();
    }

    return Payload{
        .subscription = subscription.value(),
        .event = event.value(),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace eventsub::payload::stream_offline::v1
