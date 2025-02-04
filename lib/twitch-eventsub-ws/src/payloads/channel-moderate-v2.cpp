#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

#include <unordered_map>

namespace chatterino::eventsub::lib::payload::channel_moderate::v2 {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Action, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Action>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_string())
    {
        static const error::ApplicationErrorCategory errorMustBeString{
            "Action must be a string"};
        return boost::system::error_code{129, errorMustBeString};
    }
    std::string_view eString(jvRoot.get_string());

    using namespace std::string_view_literals;
    if (eString == "ban"sv)
    {
        return Action::Ban;
    }
    if (eString == "timeout"sv)
    {
        return Action::Timeout;
    }
    if (eString == "unban"sv)
    {
        return Action::Unban;
    }
    if (eString == "untimeout"sv)
    {
        return Action::Untimeout;
    }
    if (eString == "clear"sv)
    {
        return Action::Clear;
    }
    if (eString == "emoteonly"sv)
    {
        return Action::Emoteonly;
    }
    if (eString == "emoteonlyoff"sv)
    {
        return Action::Emoteonlyoff;
    }
    if (eString == "followers"sv)
    {
        return Action::Followers;
    }
    if (eString == "followersoff"sv)
    {
        return Action::Followersoff;
    }
    if (eString == "uniquechat"sv)
    {
        return Action::Uniquechat;
    }
    if (eString == "uniquechatoff"sv)
    {
        return Action::Uniquechatoff;
    }
    if (eString == "slow"sv)
    {
        return Action::Slow;
    }
    if (eString == "slowoff"sv)
    {
        return Action::Slowoff;
    }
    if (eString == "subscribers"sv)
    {
        return Action::Subscribers;
    }
    if (eString == "subscribersoff"sv)
    {
        return Action::Subscribersoff;
    }
    if (eString == "unraid"sv)
    {
        return Action::Unraid;
    }
    if (eString == "delete"sv)
    {
        return Action::DeleteMessage;
    }
    if (eString == "unvip"sv)
    {
        return Action::Unvip;
    }
    if (eString == "vip"sv)
    {
        return Action::Vip;
    }
    if (eString == "raid"sv)
    {
        return Action::Raid;
    }
    if (eString == "add_blocked_term"sv)
    {
        return Action::AddBlockedTerm;
    }
    if (eString == "add_permitted_term"sv)
    {
        return Action::AddPermittedTerm;
    }
    if (eString == "remove_blocked_term"sv)
    {
        return Action::RemoveBlockedTerm;
    }
    if (eString == "remove_permitted_term"sv)
    {
        return Action::RemovePermittedTerm;
    }
    if (eString == "mod"sv)
    {
        return Action::Mod;
    }
    if (eString == "unmod"sv)
    {
        return Action::Unmod;
    }
    if (eString == "approve_unban_request"sv)
    {
        return Action::ApproveUnbanRequest;
    }
    if (eString == "deny_unban_request"sv)
    {
        return Action::DenyUnbanRequest;
    }
    if (eString == "warn"sv)
    {
        return Action::Warn;
    }
    if (eString == "shared_chat_ban"sv)
    {
        return Action::SharedChatBan;
    }
    if (eString == "shared_chat_timeout"sv)
    {
        return Action::SharedChatTimeout;
    }
    if (eString == "shared_chat_unban"sv)
    {
        return Action::SharedChatUnban;
    }
    if (eString == "shared_chat_untimeout"sv)
    {
        return Action::SharedChatUntimeout;
    }
    if (eString == "shared_chat_delete"sv)
    {
        return Action::SharedChatDelete;
    }

    static const error::ApplicationErrorCategory errorEnumNameDidNotExist{
        "Action did not have a constant that matched this string"};
    return boost::system::error_code{129, errorEnumNameDidNotExist};
}

boost::json::result_for<Followers, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Followers>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Followers must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvfollowDurationMinutes =
        root.if_contains("follow_duration_minutes");
    if (jvfollowDurationMinutes == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_followDurationMinutes{
                "Missing required key follow_duration_minutes"};
        return boost::system::error_code{
            129, error_missing_field_followDurationMinutes};
    }

    auto followDurationMinutes =
        boost::json::try_value_to<int>(*jvfollowDurationMinutes);

    if (followDurationMinutes.has_error())
    {
        return followDurationMinutes.error();
    }

    return Followers{
        .followDurationMinutes = std::move(followDurationMinutes.value()),
    };
}

boost::json::result_for<Slow, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Slow>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Slow must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvwaitTimeSeconds = root.if_contains("wait_time_seconds");
    if (jvwaitTimeSeconds == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_waitTimeSeconds{
                "Missing required key wait_time_seconds"};
        return boost::system::error_code{129,
                                         error_missing_field_waitTimeSeconds};
    }

    auto waitTimeSeconds = boost::json::try_value_to<int>(*jvwaitTimeSeconds);

    if (waitTimeSeconds.has_error())
    {
        return waitTimeSeconds.error();
    }

    return Slow{
        .waitTimeSeconds = std::move(waitTimeSeconds.value()),
    };
}

boost::json::result_for<Vip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Vip>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Vip must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<String>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<String>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<String>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Vip{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Unvip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unvip>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Unvip must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<String>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<String>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<String>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Unvip{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Mod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Mod>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Mod must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Mod{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Unmod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unmod>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Unmod must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Unmod{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Ban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Ban>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Ban must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

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

    auto reason = boost::json::try_value_to<std::string>(*jvreason);

    if (reason.has_error())
    {
        return reason.error();
    }

    return Ban{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .reason = std::move(reason.value()),
    };
}

boost::json::result_for<Unban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unban>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Unban must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Unban{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Timeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Timeout>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Timeout must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

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

    auto reason = boost::json::try_value_to<std::string>(*jvreason);

    if (reason.has_error())
    {
        return reason.error();
    }

    const auto *jvexpiresAt = root.if_contains("expires_at");
    if (jvexpiresAt == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_expiresAt{"Missing required key expires_at"};
        return boost::system::error_code{129, error_missing_field_expiresAt};
    }

    auto expiresAt = boost::json::try_value_to<std::string>(*jvexpiresAt);

    if (expiresAt.has_error())
    {
        return expiresAt.error();
    }

    return Timeout{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .reason = std::move(reason.value()),
        .expiresAt = std::move(expiresAt.value()),
    };
}

boost::json::result_for<Untimeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Untimeout>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Untimeout must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Untimeout{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Raid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Raid>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Raid must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    const auto *jvviewerCount = root.if_contains("viewer_count");
    if (jvviewerCount == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_viewerCount{
                "Missing required key viewer_count"};
        return boost::system::error_code{129, error_missing_field_viewerCount};
    }

    auto viewerCount = boost::json::try_value_to<int>(*jvviewerCount);

    if (viewerCount.has_error())
    {
        return viewerCount.error();
    }

    return Raid{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .viewerCount = std::move(viewerCount.value()),
    };
}

boost::json::result_for<Unraid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unraid>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Unraid must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    return Unraid{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
    };
}

boost::json::result_for<Delete, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Delete>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Delete must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

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

    const auto *jvmessageBody = root.if_contains("message_body");
    if (jvmessageBody == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_messageBody{
                "Missing required key message_body"};
        return boost::system::error_code{129, error_missing_field_messageBody};
    }

    auto messageBody = boost::json::try_value_to<std::string>(*jvmessageBody);

    if (messageBody.has_error())
    {
        return messageBody.error();
    }

    return Delete{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .messageID = std::move(messageID.value()),
        .messageBody = std::move(messageBody.value()),
    };
}

boost::json::result_for<AutomodTerms, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<AutomodTerms>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "AutomodTerms must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvaction = root.if_contains("action");
    if (jvaction == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_action{
            "Missing required key action"};
        return boost::system::error_code{129, error_missing_field_action};
    }

    auto action = boost::json::try_value_to<std::string>(*jvaction);

    if (action.has_error())
    {
        return action.error();
    }

    const auto *jvlist = root.if_contains("list");
    if (jvlist == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_list{
            "Missing required key list"};
        return boost::system::error_code{129, error_missing_field_list};
    }

    auto list = boost::json::try_value_to<std::string>(*jvlist);

    if (list.has_error())
    {
        return list.error();
    }

    const auto *jvterms = root.if_contains("terms");
    if (jvterms == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_terms{
            "Missing required key terms"};
        return boost::system::error_code{129, error_missing_field_terms};
    }
    const auto terms =
        boost::json::try_value_to<std::vector<std::string>>(*jvterms);
    if (terms.has_error())
    {
        return terms.error();
    }

    const auto *jvfromAutomod = root.if_contains("from_automod");
    if (jvfromAutomod == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_fromAutomod{
                "Missing required key from_automod"};
        return boost::system::error_code{129, error_missing_field_fromAutomod};
    }

    auto fromAutomod = boost::json::try_value_to<bool>(*jvfromAutomod);

    if (fromAutomod.has_error())
    {
        return fromAutomod.error();
    }

    return AutomodTerms{
        .action = std::move(action.value()),
        .list = std::move(list.value()),
        .terms = terms.value(),
        .fromAutomod = std::move(fromAutomod.value()),
    };
}

boost::json::result_for<UnbanRequest, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<UnbanRequest>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "UnbanRequest must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvisApproved = root.if_contains("is_approved");
    if (jvisApproved == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_isApproved{"Missing required key is_approved"};
        return boost::system::error_code{129, error_missing_field_isApproved};
    }

    auto isApproved = boost::json::try_value_to<bool>(*jvisApproved);

    if (isApproved.has_error())
    {
        return isApproved.error();
    }

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

    if (userName.has_error())
    {
        return userName.error();
    }

    const auto *jvmoderatorMessage = root.if_contains("moderator_message");
    if (jvmoderatorMessage == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_moderatorMessage{
                "Missing required key moderator_message"};
        return boost::system::error_code{129,
                                         error_missing_field_moderatorMessage};
    }

    auto moderatorMessage =
        boost::json::try_value_to<std::string>(*jvmoderatorMessage);

    if (moderatorMessage.has_error())
    {
        return moderatorMessage.error();
    }

    return UnbanRequest{
        .isApproved = std::move(isApproved.value()),
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .moderatorMessage = std::move(moderatorMessage.value()),
    };
}

boost::json::result_for<Warn, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Warn>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Warn must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvuserID = root.if_contains("user_id");
    if (jvuserID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_userID{
            "Missing required key user_id"};
        return boost::system::error_code{129, error_missing_field_userID};
    }

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

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

    auto reason = boost::json::try_value_to<std::string>(*jvreason);

    if (reason.has_error())
    {
        return reason.error();
    }

    const auto *jvchatRulesCited = root.if_contains("chat_rules_cited");
    if (jvchatRulesCited == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_chatRulesCited{
                "Missing required key chat_rules_cited"};
        return boost::system::error_code{129,
                                         error_missing_field_chatRulesCited};
    }
    const auto chatRulesCited =
        boost::json::try_value_to<std::vector<std::string>>(*jvchatRulesCited);
    if (chatRulesCited.has_error())
    {
        return chatRulesCited.error();
    }

    return Warn{
        .userID = std::move(userID.value()),
        .userLogin = std::move(userLogin.value()),
        .userName = std::move(userName.value()),
        .reason = std::move(reason.value()),
        .chatRulesCited = chatRulesCited.value(),
    };
}

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

    auto broadcasterUserID =
        boost::json::try_value_to<String>(*jvbroadcasterUserID);

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

    auto broadcasterUserLogin =
        boost::json::try_value_to<String>(*jvbroadcasterUserLogin);

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

    auto broadcasterUserName =
        boost::json::try_value_to<String>(*jvbroadcasterUserName);

    if (broadcasterUserName.has_error())
    {
        return broadcasterUserName.error();
    }

    std::optional<std::string> sourceBroadcasterUserID = std::nullopt;
    const auto *jvsourceBroadcasterUserID =
        root.if_contains("source_broadcaster_user_id");
    if (jvsourceBroadcasterUserID != nullptr &&
        !jvsourceBroadcasterUserID->is_null())
    {
        auto tsourceBroadcasterUserID =
            boost::json::try_value_to<std::string>(*jvsourceBroadcasterUserID);

        if (tsourceBroadcasterUserID.has_error())
        {
            return tsourceBroadcasterUserID.error();
        }
        sourceBroadcasterUserID = std::move(tsourceBroadcasterUserID.value());
    }

    std::optional<std::string> sourceBroadcasterUserLogin = std::nullopt;
    const auto *jvsourceBroadcasterUserLogin =
        root.if_contains("source_broadcaster_user_login");
    if (jvsourceBroadcasterUserLogin != nullptr &&
        !jvsourceBroadcasterUserLogin->is_null())
    {
        auto tsourceBroadcasterUserLogin =
            boost::json::try_value_to<std::string>(
                *jvsourceBroadcasterUserLogin);

        if (tsourceBroadcasterUserLogin.has_error())
        {
            return tsourceBroadcasterUserLogin.error();
        }
        sourceBroadcasterUserLogin =
            std::move(tsourceBroadcasterUserLogin.value());
    }

    std::optional<std::string> sourceBroadcasterUserName = std::nullopt;
    const auto *jvsourceBroadcasterUserName =
        root.if_contains("source_broadcaster_user_name");
    if (jvsourceBroadcasterUserName != nullptr &&
        !jvsourceBroadcasterUserName->is_null())
    {
        auto tsourceBroadcasterUserName =
            boost::json::try_value_to<std::string>(
                *jvsourceBroadcasterUserName);

        if (tsourceBroadcasterUserName.has_error())
        {
            return tsourceBroadcasterUserName.error();
        }
        sourceBroadcasterUserName =
            std::move(tsourceBroadcasterUserName.value());
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

    auto moderatorUserID =
        boost::json::try_value_to<String>(*jvmoderatorUserID);

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

    auto moderatorUserLogin =
        boost::json::try_value_to<String>(*jvmoderatorUserLogin);

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

    auto moderatorUserName =
        boost::json::try_value_to<String>(*jvmoderatorUserName);

    if (moderatorUserName.has_error())
    {
        return moderatorUserName.error();
    }

    const auto *jvaction = root.if_contains("action");
    if (jvaction == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_action{
            "Missing required key action"};
        return boost::system::error_code{129, error_missing_field_action};
    }

    auto action = boost::json::try_value_to<Action>(*jvaction);

    if (action.has_error())
    {
        return action.error();
    }

    std::optional<Followers> followers = std::nullopt;
    const auto *jvfollowers = root.if_contains("followers");
    if (jvfollowers != nullptr && !jvfollowers->is_null())
    {
        auto tfollowers = boost::json::try_value_to<Followers>(*jvfollowers);

        if (tfollowers.has_error())
        {
            return tfollowers.error();
        }
        followers = std::move(tfollowers.value());
    }

    std::optional<Slow> slow = std::nullopt;
    const auto *jvslow = root.if_contains("slow");
    if (jvslow != nullptr && !jvslow->is_null())
    {
        auto tslow = boost::json::try_value_to<Slow>(*jvslow);

        if (tslow.has_error())
        {
            return tslow.error();
        }
        slow = std::move(tslow.value());
    }

    std::optional<Vip> vip = std::nullopt;
    const auto *jvvip = root.if_contains("vip");
    if (jvvip != nullptr && !jvvip->is_null())
    {
        auto tvip = boost::json::try_value_to<Vip>(*jvvip);

        if (tvip.has_error())
        {
            return tvip.error();
        }
        vip = std::move(tvip.value());
    }

    std::optional<Unvip> unvip = std::nullopt;
    const auto *jvunvip = root.if_contains("unvip");
    if (jvunvip != nullptr && !jvunvip->is_null())
    {
        auto tunvip = boost::json::try_value_to<Unvip>(*jvunvip);

        if (tunvip.has_error())
        {
            return tunvip.error();
        }
        unvip = std::move(tunvip.value());
    }

    std::optional<Unmod> unmod = std::nullopt;
    const auto *jvunmod = root.if_contains("unmod");
    if (jvunmod != nullptr && !jvunmod->is_null())
    {
        auto tunmod = boost::json::try_value_to<Unmod>(*jvunmod);

        if (tunmod.has_error())
        {
            return tunmod.error();
        }
        unmod = std::move(tunmod.value());
    }

    std::optional<Ban> ban = std::nullopt;
    const auto *jvban = root.if_contains("ban");
    if (jvban != nullptr && !jvban->is_null())
    {
        auto tban = boost::json::try_value_to<Ban>(*jvban);

        if (tban.has_error())
        {
            return tban.error();
        }
        ban = std::move(tban.value());
    }

    std::optional<Unban> unban = std::nullopt;
    const auto *jvunban = root.if_contains("unban");
    if (jvunban != nullptr && !jvunban->is_null())
    {
        auto tunban = boost::json::try_value_to<Unban>(*jvunban);

        if (tunban.has_error())
        {
            return tunban.error();
        }
        unban = std::move(tunban.value());
    }

    std::optional<Timeout> timeout = std::nullopt;
    const auto *jvtimeout = root.if_contains("timeout");
    if (jvtimeout != nullptr && !jvtimeout->is_null())
    {
        auto ttimeout = boost::json::try_value_to<Timeout>(*jvtimeout);

        if (ttimeout.has_error())
        {
            return ttimeout.error();
        }
        timeout = std::move(ttimeout.value());
    }

    std::optional<Untimeout> untimeout = std::nullopt;
    const auto *jvuntimeout = root.if_contains("untimeout");
    if (jvuntimeout != nullptr && !jvuntimeout->is_null())
    {
        auto tuntimeout = boost::json::try_value_to<Untimeout>(*jvuntimeout);

        if (tuntimeout.has_error())
        {
            return tuntimeout.error();
        }
        untimeout = std::move(tuntimeout.value());
    }

    std::optional<Raid> raid = std::nullopt;
    const auto *jvraid = root.if_contains("raid");
    if (jvraid != nullptr && !jvraid->is_null())
    {
        auto traid = boost::json::try_value_to<Raid>(*jvraid);

        if (traid.has_error())
        {
            return traid.error();
        }
        raid = std::move(traid.value());
    }

    std::optional<Unraid> unraid = std::nullopt;
    const auto *jvunraid = root.if_contains("unraid");
    if (jvunraid != nullptr && !jvunraid->is_null())
    {
        auto tunraid = boost::json::try_value_to<Unraid>(*jvunraid);

        if (tunraid.has_error())
        {
            return tunraid.error();
        }
        unraid = std::move(tunraid.value());
    }

    std::optional<Delete> deleteMessage = std::nullopt;
    const auto *jvdeleteMessage = root.if_contains("delete");
    if (jvdeleteMessage != nullptr && !jvdeleteMessage->is_null())
    {
        auto tdeleteMessage =
            boost::json::try_value_to<Delete>(*jvdeleteMessage);

        if (tdeleteMessage.has_error())
        {
            return tdeleteMessage.error();
        }
        deleteMessage = std::move(tdeleteMessage.value());
    }

    std::optional<AutomodTerms> automodTerms = std::nullopt;
    const auto *jvautomodTerms = root.if_contains("automod_terms");
    if (jvautomodTerms != nullptr && !jvautomodTerms->is_null())
    {
        auto tautomodTerms =
            boost::json::try_value_to<AutomodTerms>(*jvautomodTerms);

        if (tautomodTerms.has_error())
        {
            return tautomodTerms.error();
        }
        automodTerms = std::move(tautomodTerms.value());
    }

    std::optional<UnbanRequest> unbanRequest = std::nullopt;
    const auto *jvunbanRequest = root.if_contains("unban_request");
    if (jvunbanRequest != nullptr && !jvunbanRequest->is_null())
    {
        auto tunbanRequest =
            boost::json::try_value_to<UnbanRequest>(*jvunbanRequest);

        if (tunbanRequest.has_error())
        {
            return tunbanRequest.error();
        }
        unbanRequest = std::move(tunbanRequest.value());
    }

    std::optional<Warn> warn = std::nullopt;
    const auto *jvwarn = root.if_contains("warn");
    if (jvwarn != nullptr && !jvwarn->is_null())
    {
        auto twarn = boost::json::try_value_to<Warn>(*jvwarn);

        if (twarn.has_error())
        {
            return twarn.error();
        }
        warn = std::move(twarn.value());
    }

    std::optional<Ban> sharedChatBan = std::nullopt;
    const auto *jvsharedChatBan = root.if_contains("shared_chat_ban");
    if (jvsharedChatBan != nullptr && !jvsharedChatBan->is_null())
    {
        auto tsharedChatBan = boost::json::try_value_to<Ban>(*jvsharedChatBan);

        if (tsharedChatBan.has_error())
        {
            return tsharedChatBan.error();
        }
        sharedChatBan = std::move(tsharedChatBan.value());
    }

    std::optional<Unban> sharedChatUnban = std::nullopt;
    const auto *jvsharedChatUnban = root.if_contains("shared_chat_unban");
    if (jvsharedChatUnban != nullptr && !jvsharedChatUnban->is_null())
    {
        auto tsharedChatUnban =
            boost::json::try_value_to<Unban>(*jvsharedChatUnban);

        if (tsharedChatUnban.has_error())
        {
            return tsharedChatUnban.error();
        }
        sharedChatUnban = std::move(tsharedChatUnban.value());
    }

    std::optional<Timeout> sharedChatTimeout = std::nullopt;
    const auto *jvsharedChatTimeout = root.if_contains("shared_chat_timeout");
    if (jvsharedChatTimeout != nullptr && !jvsharedChatTimeout->is_null())
    {
        auto tsharedChatTimeout =
            boost::json::try_value_to<Timeout>(*jvsharedChatTimeout);

        if (tsharedChatTimeout.has_error())
        {
            return tsharedChatTimeout.error();
        }
        sharedChatTimeout = std::move(tsharedChatTimeout.value());
    }

    std::optional<Untimeout> sharedChatUntimeout = std::nullopt;
    const auto *jvsharedChatUntimeout =
        root.if_contains("shared_chat_untimeout");
    if (jvsharedChatUntimeout != nullptr && !jvsharedChatUntimeout->is_null())
    {
        auto tsharedChatUntimeout =
            boost::json::try_value_to<Untimeout>(*jvsharedChatUntimeout);

        if (tsharedChatUntimeout.has_error())
        {
            return tsharedChatUntimeout.error();
        }
        sharedChatUntimeout = std::move(tsharedChatUntimeout.value());
    }

    std::optional<Delete> sharedChatDelete = std::nullopt;
    const auto *jvsharedChatDelete = root.if_contains("shared_chat_delete");
    if (jvsharedChatDelete != nullptr && !jvsharedChatDelete->is_null())
    {
        auto tsharedChatDelete =
            boost::json::try_value_to<Delete>(*jvsharedChatDelete);

        if (tsharedChatDelete.has_error())
        {
            return tsharedChatDelete.error();
        }
        sharedChatDelete = std::move(tsharedChatDelete.value());
    }

    return Event{
        .broadcasterUserID = std::move(broadcasterUserID.value()),
        .broadcasterUserLogin = std::move(broadcasterUserLogin.value()),
        .broadcasterUserName = std::move(broadcasterUserName.value()),
        .sourceBroadcasterUserID = std::move(sourceBroadcasterUserID),
        .sourceBroadcasterUserLogin = std::move(sourceBroadcasterUserLogin),
        .sourceBroadcasterUserName = std::move(sourceBroadcasterUserName),
        .moderatorUserID = std::move(moderatorUserID.value()),
        .moderatorUserLogin = std::move(moderatorUserLogin.value()),
        .moderatorUserName = std::move(moderatorUserName.value()),
        .action = std::move(action.value()),
        .followers = std::move(followers),
        .slow = std::move(slow),
        .vip = std::move(vip),
        .unvip = std::move(unvip),
        .unmod = std::move(unmod),
        .ban = std::move(ban),
        .unban = std::move(unban),
        .timeout = std::move(timeout),
        .untimeout = std::move(untimeout),
        .raid = std::move(raid),
        .unraid = std::move(unraid),
        .deleteMessage = std::move(deleteMessage),
        .automodTerms = std::move(automodTerms),
        .unbanRequest = std::move(unbanRequest),
        .warn = std::move(warn),
        .sharedChatBan = std::move(sharedChatBan),
        .sharedChatUnban = std::move(sharedChatUnban),
        .sharedChatTimeout = std::move(sharedChatTimeout),
        .sharedChatUntimeout = std::move(sharedChatUntimeout),
        .sharedChatDelete = std::move(sharedChatDelete),
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

    auto subscription =
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

    auto event = boost::json::try_value_to<Event>(*jvevent);

    if (event.has_error())
    {
        return event.error();
    }

    return Payload{
        .subscription = std::move(subscription.value()),
        .event = std::move(event.value()),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace chatterino::eventsub::lib::payload::channel_moderate::v2
