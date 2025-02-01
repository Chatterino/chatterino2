#include "twitch-eventsub-ws/payloads/channel-ban-v1.hpp"

#include "twitch-eventsub-ws/chrono.hpp"
#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::payload::channel_ban::v1 {

std::chrono::system_clock::duration Event::timeoutDuration() const
{
    if (!this->endsAt)
    {
        return {};
    }

    return *this->endsAt - this->bannedAt;
}

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

    const auto *jvmoderatorUserID = root.if_contains("moderator_user_id");
    if (jvmoderatorUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_moderatorUserID{
                "Missing required key moderator_user_id"};
        return boost::system::error_code{129,
                                         error_missing_field_moderatorUserID};
    }

    const auto moderatorUserID =
        boost::json::try_value_to<std::string>(*jvmoderatorUserID);

    if (moderatorUserID.has_error())
    {
        return moderatorUserID.error();
    }

    const auto *jvmoderatorUserLogin = root.if_contains("moderator_user_login");
    if (jvmoderatorUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_moderatorUserLogin{
                "Missing required key moderator_user_login"};
        return boost::system::error_code{
            129, error_missing_field_moderatorUserLogin};
    }

    const auto moderatorUserLogin =
        boost::json::try_value_to<std::string>(*jvmoderatorUserLogin);

    if (moderatorUserLogin.has_error())
    {
        return moderatorUserLogin.error();
    }

    const auto *jvmoderatorUserName = root.if_contains("moderator_user_name");
    if (jvmoderatorUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_moderatorUserName{
                "Missing required key moderator_user_name"};
        return boost::system::error_code{129,
                                         error_missing_field_moderatorUserName};
    }

    const auto moderatorUserName =
        boost::json::try_value_to<std::string>(*jvmoderatorUserName);

    if (moderatorUserName.has_error())
    {
        return moderatorUserName.error();
    }

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    const auto userID = boost::json::try_value_to<std::string>(*jvuserID);

    if (userID.has_error())
    {
        return userID.error();
    }

    const auto *jvuserLogin = root.if_contains("user_login");
    if (jvuserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_userLogin{"Missing required key user_login"};
        return boost::system::error_code{129, error_missing_field_userLogin};
    }

    const auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

    if (userLogin.has_error())
    {
        return userLogin.error();
    }

    const auto *jvuserName = root.if_contains("user_name");
    if (jvuserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_userName{"Missing required key user_name"};
        return boost::system::error_code{129, error_missing_field_userName};
    }

    const auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    const auto *jvreason = root.if_contains("reason");
    if (jvreason == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_reason{
            "Missing required key reason"};
        return boost::system::error_code{129, error_missing_field_reason};
    }

    const auto reason = boost::json::try_value_to<std::string>(*jvreason);

    if (reason.has_error())
    {
        return reason.error();
    }

    const auto *jvisPermanent = root.if_contains("is_permanent");
    if (jvisPermanent == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_isPermanent{
                "Missing required key is_permanent"};
        return boost::system::error_code{129, error_missing_field_isPermanent};
    }

    const auto isPermanent = boost::json::try_value_to<bool>(*jvisPermanent);

    if (isPermanent.has_error())
    {
        return isPermanent.error();
    }

    const auto *jvbannedAt = root.if_contains("banned_at");
    if (jvbannedAt == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_bannedAt{"Missing required key banned_at"};
        return boost::system::error_code{129, error_missing_field_bannedAt};
    }

    const auto bannedAt =
        boost::json::try_value_to<std::chrono::system_clock::time_point>(
            *jvbannedAt, AsISO8601());

    if (bannedAt.has_error())
    {
        return bannedAt.error();
    }

    std::optional<std::chrono::system_clock::time_point> endsAt = std::nullopt;
    const auto *jvendsAt = root.if_contains("ends_at");
    if (jvendsAt != nullptr && !jvendsAt->is_null())
    {
        const auto tendsAt =
            boost::json::try_value_to<std::chrono::system_clock::time_point>(
                *jvendsAt, AsISO8601());

        if (tendsAt.has_error())
        {
            return tendsAt.error();
        }
        endsAt = tendsAt.value();
    }

    return Event{
        .broadcasterUserID = broadcasterUserID.value(),
        .broadcasterUserLogin = broadcasterUserLogin.value(),
        .broadcasterUserName = broadcasterUserName.value(),
        .moderatorUserID = moderatorUserID.value(),
        .moderatorUserLogin = moderatorUserLogin.value(),
        .moderatorUserName = moderatorUserName.value(),
        .userID = userID.value(),
        .userLogin = userLogin.value(),
        .userName = userName.value(),
        .reason = reason.value(),
        .isPermanent = isPermanent.value(),
        .bannedAt = bannedAt.value(),
        .endsAt = endsAt,
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

}  // namespace eventsub::payload::channel_ban::v1
