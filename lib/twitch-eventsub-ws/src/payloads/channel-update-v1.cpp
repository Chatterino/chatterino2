#include "twitch-eventsub-ws/payloads/channel-update-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::payload::channel_update::v1 {

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

    const auto *jvtitle = root.if_contains("title");
    if (jvtitle == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_title{
            "Missing required key title"};
        return boost::system::error_code{129, error_missing_field_title};
    }

    const auto title = boost::json::try_value_to<std::string>(*jvtitle);

    if (title.has_error())
    {
        return title.error();
    }

    const auto *jvlanguage = root.if_contains("language");
    if (jvlanguage == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_language{"Missing required key language"};
        return boost::system::error_code{129, error_missing_field_language};
    }

    const auto language = boost::json::try_value_to<std::string>(*jvlanguage);

    if (language.has_error())
    {
        return language.error();
    }

    const auto *jvcategoryID = root.if_contains("category_id");
    if (jvcategoryID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_categoryID{"Missing required key category_id"};
        return boost::system::error_code{129, error_missing_field_categoryID};
    }

    const auto categoryID =
        boost::json::try_value_to<std::string>(*jvcategoryID);

    if (categoryID.has_error())
    {
        return categoryID.error();
    }

    const auto *jvcategoryName = root.if_contains("category_name");
    if (jvcategoryName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_categoryName{
                "Missing required key category_name"};
        return boost::system::error_code{129, error_missing_field_categoryName};
    }

    const auto categoryName =
        boost::json::try_value_to<std::string>(*jvcategoryName);

    if (categoryName.has_error())
    {
        return categoryName.error();
    }

    const auto *jvisMature = root.if_contains("is_mature");
    if (jvisMature == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_isMature{"Missing required key is_mature"};
        return boost::system::error_code{129, error_missing_field_isMature};
    }

    const auto isMature = boost::json::try_value_to<bool>(*jvisMature);

    if (isMature.has_error())
    {
        return isMature.error();
    }

    return Event{
        .broadcasterUserID = broadcasterUserID.value(),
        .broadcasterUserLogin = broadcasterUserLogin.value(),
        .broadcasterUserName = broadcasterUserName.value(),
        .title = title.value(),
        .language = language.value(),
        .categoryID = categoryID.value(),
        .categoryName = categoryName.value(),
        .isMature = isMature.value(),
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

}  // namespace eventsub::payload::channel_update::v1
