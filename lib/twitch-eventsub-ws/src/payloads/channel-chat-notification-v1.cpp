#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::payload::channel_chat_notification::v1 {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Badge, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Badge>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Badge must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvsetID = root.if_contains("set_id");
    if (jvsetID == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_setID{
            "Missing required key set_id"};
        return boost::system::error_code{129, error_missing_field_setID};
    }

    const auto setID = boost::json::try_value_to<std::string>(*jvsetID);

    if (setID.has_error())
    {
        return setID.error();
    }

    const auto *jvid = root.if_contains("id");
    if (jvid == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_id{
            "Missing required key id"};
        return boost::system::error_code{129, error_missing_field_id};
    }

    const auto id = boost::json::try_value_to<std::string>(*jvid);

    if (id.has_error())
    {
        return id.error();
    }

    const auto *jvinfo = root.if_contains("info");
    if (jvinfo == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_info{
            "Missing required key info"};
        return boost::system::error_code{129, error_missing_field_info};
    }

    const auto info = boost::json::try_value_to<std::string>(*jvinfo);

    if (info.has_error())
    {
        return info.error();
    }

    return Badge{
        .setID = setID.value(),
        .id = id.value(),
        .info = info.value(),
    };
}

boost::json::result_for<Cheermote, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Cheermote>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Cheermote must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvprefix = root.if_contains("prefix");
    if (jvprefix == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_prefix{
            "Missing required key prefix"};
        return boost::system::error_code{129, error_missing_field_prefix};
    }

    const auto prefix = boost::json::try_value_to<std::string>(*jvprefix);

    if (prefix.has_error())
    {
        return prefix.error();
    }

    const auto *jvbits = root.if_contains("bits");
    if (jvbits == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_bits{
            "Missing required key bits"};
        return boost::system::error_code{129, error_missing_field_bits};
    }

    const auto bits = boost::json::try_value_to<int>(*jvbits);

    if (bits.has_error())
    {
        return bits.error();
    }

    const auto *jvtier = root.if_contains("tier");
    if (jvtier == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_tier{
            "Missing required key tier"};
        return boost::system::error_code{129, error_missing_field_tier};
    }

    const auto tier = boost::json::try_value_to<int>(*jvtier);

    if (tier.has_error())
    {
        return tier.error();
    }

    return Cheermote{
        .prefix = prefix.value(),
        .bits = bits.value(),
        .tier = tier.value(),
    };
}

boost::json::result_for<Emote, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Emote>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Emote must be an object"};
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

    const auto id = boost::json::try_value_to<std::string>(*jvid);

    if (id.has_error())
    {
        return id.error();
    }

    const auto *jvemoteSetID = root.if_contains("emote_set_id");
    if (jvemoteSetID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_emoteSetID{"Missing required key emote_set_id"};
        return boost::system::error_code{129, error_missing_field_emoteSetID};
    }

    const auto emoteSetID =
        boost::json::try_value_to<std::string>(*jvemoteSetID);

    if (emoteSetID.has_error())
    {
        return emoteSetID.error();
    }

    const auto *jvownerID = root.if_contains("owner_id");
    if (jvownerID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_ownerID{"Missing required key owner_id"};
        return boost::system::error_code{129, error_missing_field_ownerID};
    }

    const auto ownerID = boost::json::try_value_to<std::string>(*jvownerID);

    if (ownerID.has_error())
    {
        return ownerID.error();
    }

    const auto *jvformat = root.if_contains("format");
    if (jvformat == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_format{
            "Missing required key format"};
        return boost::system::error_code{129, error_missing_field_format};
    }
    const auto format =
        boost::json::try_value_to<std::vector<std::string>>(*jvformat);
    if (format.has_error())
    {
        return format.error();
    }

    return Emote{
        .id = id.value(),
        .emoteSetID = emoteSetID.value(),
        .ownerID = ownerID.value(),
        .format = format.value(),
    };
}

boost::json::result_for<Mention, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Mention>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Mention must be an object"};
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

    const auto userID = boost::json::try_value_to<std::string>(*jvuserID);

    if (userID.has_error())
    {
        return userID.error();
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

    return Mention{
        .userID = userID.value(),
        .userName = userName.value(),
        .userLogin = userLogin.value(),
    };
}

boost::json::result_for<MessageFragment, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<MessageFragment>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "MessageFragment must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvtype = root.if_contains("type");
    if (jvtype == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_type{
            "Missing required key type"};
        return boost::system::error_code{129, error_missing_field_type};
    }

    const auto type = boost::json::try_value_to<std::string>(*jvtype);

    if (type.has_error())
    {
        return type.error();
    }

    const auto *jvtext = root.if_contains("text");
    if (jvtext == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_text{
            "Missing required key text"};
        return boost::system::error_code{129, error_missing_field_text};
    }

    const auto text = boost::json::try_value_to<std::string>(*jvtext);

    if (text.has_error())
    {
        return text.error();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Cheermote>
        cheermote = std::nullopt;
    const auto *jvcheermote = root.if_contains("cheermote");
    if (jvcheermote != nullptr && !jvcheermote->is_null())
    {
        const auto tcheermote = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Cheermote>(
            *jvcheermote);

        if (tcheermote.has_error())
        {
            return tcheermote.error();
        }
        cheermote = tcheermote.value();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Emote>
        emote = std::nullopt;
    const auto *jvemote = root.if_contains("emote");
    if (jvemote != nullptr && !jvemote->is_null())
    {
        const auto temote = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Emote>(*jvemote);

        if (temote.has_error())
        {
            return temote.error();
        }
        emote = temote.value();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Mention>
        mention = std::nullopt;
    const auto *jvmention = root.if_contains("mention");
    if (jvmention != nullptr && !jvmention->is_null())
    {
        const auto tmention = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Mention>(
            *jvmention);

        if (tmention.has_error())
        {
            return tmention.error();
        }
        mention = tmention.value();
    }

    return MessageFragment{
        .type = type.value(),
        .text = text.value(),
        .cheermote = cheermote,
        .emote = emote,
        .mention = mention,
    };
}

boost::json::result_for<Subcription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subcription>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Subcription must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    const auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    const auto *jvisPrime = root.if_contains("is_prime");
    if (jvisPrime == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_isPrime{"Missing required key is_prime"};
        return boost::system::error_code{129, error_missing_field_isPrime};
    }

    const auto isPrime = boost::json::try_value_to<bool>(*jvisPrime);

    if (isPrime.has_error())
    {
        return isPrime.error();
    }

    const auto *jvdurationMonths = root.if_contains("duration_months");
    if (jvdurationMonths == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_durationMonths{
                "Missing required key duration_months"};
        return boost::system::error_code{129,
                                         error_missing_field_durationMonths};
    }

    const auto durationMonths =
        boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    return Subcription{
        .subTier = subTier.value(),
        .isPrime = isPrime.value(),
        .durationMonths = durationMonths.value(),
    };
}

boost::json::result_for<Resubscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Resubscription>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Resubscription must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvcumulativeMonths = root.if_contains("cumulative_months");
    if (jvcumulativeMonths == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_cumulativeMonths{
                "Missing required key cumulative_months"};
        return boost::system::error_code{129,
                                         error_missing_field_cumulativeMonths};
    }

    const auto cumulativeMonths =
        boost::json::try_value_to<int>(*jvcumulativeMonths);

    if (cumulativeMonths.has_error())
    {
        return cumulativeMonths.error();
    }

    const auto *jvdurationMonths = root.if_contains("duration_months");
    if (jvdurationMonths == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_durationMonths{
                "Missing required key duration_months"};
        return boost::system::error_code{129,
                                         error_missing_field_durationMonths};
    }

    const auto durationMonths =
        boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    std::optional<int> streakMonths = std::nullopt;
    const auto *jvstreakMonths = root.if_contains("streak_months");
    if (jvstreakMonths != nullptr && !jvstreakMonths->is_null())
    {
        const auto tstreakMonths =
            boost::json::try_value_to<int>(*jvstreakMonths);

        if (tstreakMonths.has_error())
        {
            return tstreakMonths.error();
        }
        streakMonths = tstreakMonths.value();
    }

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    const auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    const auto *jvisPrime = root.if_contains("is_prime");
    if (jvisPrime == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_isPrime{"Missing required key is_prime"};
        return boost::system::error_code{129, error_missing_field_isPrime};
    }

    const auto isPrime = boost::json::try_value_to<bool>(*jvisPrime);

    if (isPrime.has_error())
    {
        return isPrime.error();
    }

    const auto *jvisGift = root.if_contains("is_gift");
    if (jvisGift == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_isGift{
            "Missing required key is_gift"};
        return boost::system::error_code{129, error_missing_field_isGift};
    }

    const auto isGift = boost::json::try_value_to<bool>(*jvisGift);

    if (isGift.has_error())
    {
        return isGift.error();
    }

    const auto *jvgifterIsAnonymous = root.if_contains("gifter_is_anonymous");
    if (jvgifterIsAnonymous == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_gifterIsAnonymous{
                "Missing required key gifter_is_anonymous"};
        return boost::system::error_code{129,
                                         error_missing_field_gifterIsAnonymous};
    }

    const auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        const auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = tgifterUserID.value();
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        const auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = tgifterUserName.value();
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        const auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = tgifterUserLogin.value();
    }

    return Resubscription{
        .cumulativeMonths = cumulativeMonths.value(),
        .durationMonths = durationMonths.value(),
        .streakMonths = streakMonths,
        .subTier = subTier.value(),
        .isPrime = isPrime.value(),
        .isGift = isGift.value(),
        .gifterIsAnonymous = gifterIsAnonymous.value(),
        .gifterUserID = gifterUserID,
        .gifterUserName = gifterUserName,
        .gifterUserLogin = gifterUserLogin,
    };
}

boost::json::result_for<GiftSubscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<GiftSubscription>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "GiftSubscription must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvdurationMonths = root.if_contains("duration_months");
    if (jvdurationMonths == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_durationMonths{
                "Missing required key duration_months"};
        return boost::system::error_code{129,
                                         error_missing_field_durationMonths};
    }

    const auto durationMonths =
        boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    std::optional<int> cumulativeTotal = std::nullopt;
    const auto *jvcumulativeTotal = root.if_contains("cumulative_total");
    if (jvcumulativeTotal != nullptr && !jvcumulativeTotal->is_null())
    {
        const auto tcumulativeTotal =
            boost::json::try_value_to<int>(*jvcumulativeTotal);

        if (tcumulativeTotal.has_error())
        {
            return tcumulativeTotal.error();
        }
        cumulativeTotal = tcumulativeTotal.value();
    }

    std::optional<int> streakMonths = std::nullopt;
    const auto *jvstreakMonths = root.if_contains("streak_months");
    if (jvstreakMonths != nullptr && !jvstreakMonths->is_null())
    {
        const auto tstreakMonths =
            boost::json::try_value_to<int>(*jvstreakMonths);

        if (tstreakMonths.has_error())
        {
            return tstreakMonths.error();
        }
        streakMonths = tstreakMonths.value();
    }

    const auto *jvrecipientUserID = root.if_contains("recipient_user_id");
    if (jvrecipientUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_recipientUserID{
                "Missing required key recipient_user_id"};
        return boost::system::error_code{129,
                                         error_missing_field_recipientUserID};
    }

    const auto recipientUserID =
        boost::json::try_value_to<std::string>(*jvrecipientUserID);

    if (recipientUserID.has_error())
    {
        return recipientUserID.error();
    }

    const auto *jvrecipientUserName = root.if_contains("recipient_user_name");
    if (jvrecipientUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_recipientUserName{
                "Missing required key recipient_user_name"};
        return boost::system::error_code{129,
                                         error_missing_field_recipientUserName};
    }

    const auto recipientUserName =
        boost::json::try_value_to<std::string>(*jvrecipientUserName);

    if (recipientUserName.has_error())
    {
        return recipientUserName.error();
    }

    const auto *jvrecipientUserLogin = root.if_contains("recipient_user_login");
    if (jvrecipientUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_recipientUserLogin{
                "Missing required key recipient_user_login"};
        return boost::system::error_code{
            129, error_missing_field_recipientUserLogin};
    }

    const auto recipientUserLogin =
        boost::json::try_value_to<std::string>(*jvrecipientUserLogin);

    if (recipientUserLogin.has_error())
    {
        return recipientUserLogin.error();
    }

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    const auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    std::optional<std::string> communityGiftID = std::nullopt;
    const auto *jvcommunityGiftID = root.if_contains("community_gift_id");
    if (jvcommunityGiftID != nullptr && !jvcommunityGiftID->is_null())
    {
        const auto tcommunityGiftID =
            boost::json::try_value_to<std::string>(*jvcommunityGiftID);

        if (tcommunityGiftID.has_error())
        {
            return tcommunityGiftID.error();
        }
        communityGiftID = tcommunityGiftID.value();
    }

    return GiftSubscription{
        .durationMonths = durationMonths.value(),
        .cumulativeTotal = cumulativeTotal,
        .streakMonths = streakMonths,
        .recipientUserID = recipientUserID.value(),
        .recipientUserName = recipientUserName.value(),
        .recipientUserLogin = recipientUserLogin.value(),
        .subTier = subTier.value(),
        .communityGiftID = communityGiftID,
    };
}

boost::json::result_for<CommunityGiftSubscription, boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<CommunityGiftSubscription>,
               const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "CommunityGiftSubscription must be an object"};
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

    const auto id = boost::json::try_value_to<std::string>(*jvid);

    if (id.has_error())
    {
        return id.error();
    }

    const auto *jvtotal = root.if_contains("total");
    if (jvtotal == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_total{
            "Missing required key total"};
        return boost::system::error_code{129, error_missing_field_total};
    }

    const auto total = boost::json::try_value_to<int>(*jvtotal);

    if (total.has_error())
    {
        return total.error();
    }

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    const auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    std::optional<int> cumulativeTotal = std::nullopt;
    const auto *jvcumulativeTotal = root.if_contains("cumulative_total");
    if (jvcumulativeTotal != nullptr && !jvcumulativeTotal->is_null())
    {
        const auto tcumulativeTotal =
            boost::json::try_value_to<int>(*jvcumulativeTotal);

        if (tcumulativeTotal.has_error())
        {
            return tcumulativeTotal.error();
        }
        cumulativeTotal = tcumulativeTotal.value();
    }

    return CommunityGiftSubscription{
        .id = id.value(),
        .total = total.value(),
        .subTier = subTier.value(),
        .cumulativeTotal = cumulativeTotal,
    };
}

boost::json::result_for<GiftPaidUpgrade, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<GiftPaidUpgrade>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "GiftPaidUpgrade must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvgifterIsAnonymous = root.if_contains("gifter_is_anonymous");
    if (jvgifterIsAnonymous == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_gifterIsAnonymous{
                "Missing required key gifter_is_anonymous"};
        return boost::system::error_code{129,
                                         error_missing_field_gifterIsAnonymous};
    }

    const auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        const auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = tgifterUserID.value();
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        const auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = tgifterUserName.value();
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        const auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = tgifterUserLogin.value();
    }

    return GiftPaidUpgrade{
        .gifterIsAnonymous = gifterIsAnonymous.value(),
        .gifterUserID = gifterUserID,
        .gifterUserName = gifterUserName,
        .gifterUserLogin = gifterUserLogin,
    };
}

boost::json::result_for<PrimePaidUpgrade, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<PrimePaidUpgrade>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "PrimePaidUpgrade must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    const auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    return PrimePaidUpgrade{
        .subTier = subTier.value(),
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

    const auto userID = boost::json::try_value_to<std::string>(*jvuserID);

    if (userID.has_error())
    {
        return userID.error();
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

    const auto *jvviewerCount = root.if_contains("viewer_count");
    if (jvviewerCount == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_viewerCount{
                "Missing required key viewer_count"};
        return boost::system::error_code{129, error_missing_field_viewerCount};
    }

    const auto viewerCount = boost::json::try_value_to<int>(*jvviewerCount);

    if (viewerCount.has_error())
    {
        return viewerCount.error();
    }

    const auto *jvprofileImageURL = root.if_contains("profile_image_url");
    if (jvprofileImageURL == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_profileImageURL{
                "Missing required key profile_image_url"};
        return boost::system::error_code{129,
                                         error_missing_field_profileImageURL};
    }

    const auto profileImageURL =
        boost::json::try_value_to<std::string>(*jvprofileImageURL);

    if (profileImageURL.has_error())
    {
        return profileImageURL.error();
    }

    return Raid{
        .userID = userID.value(),
        .userName = userName.value(),
        .userLogin = userLogin.value(),
        .viewerCount = viewerCount.value(),
        .profileImageURL = profileImageURL.value(),
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

    return Unraid{};
}

boost::json::result_for<PayItForward, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<PayItForward>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "PayItForward must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvgifterIsAnonymous = root.if_contains("gifter_is_anonymous");
    if (jvgifterIsAnonymous == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_gifterIsAnonymous{
                "Missing required key gifter_is_anonymous"};
        return boost::system::error_code{129,
                                         error_missing_field_gifterIsAnonymous};
    }

    const auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        const auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = tgifterUserID.value();
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        const auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = tgifterUserName.value();
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        const auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = tgifterUserLogin.value();
    }

    return PayItForward{
        .gifterIsAnonymous = gifterIsAnonymous.value(),
        .gifterUserID = gifterUserID,
        .gifterUserName = gifterUserName,
        .gifterUserLogin = gifterUserLogin,
    };
}

boost::json::result_for<Announcement, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Announcement>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Announcement must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvcolor = root.if_contains("color");
    if (jvcolor == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_color{
            "Missing required key color"};
        return boost::system::error_code{129, error_missing_field_color};
    }

    const auto color = boost::json::try_value_to<std::string>(*jvcolor);

    if (color.has_error())
    {
        return color.error();
    }

    return Announcement{
        .color = color.value(),
    };
}

boost::json::result_for<CharityDonationAmount, boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<CharityDonationAmount>,
               const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "CharityDonationAmount must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvvalue = root.if_contains("value");
    if (jvvalue == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_value{
            "Missing required key value"};
        return boost::system::error_code{129, error_missing_field_value};
    }

    const auto value = boost::json::try_value_to<int>(*jvvalue);

    if (value.has_error())
    {
        return value.error();
    }

    const auto *jvdecimalPlaces = root.if_contains("decimal_places");
    if (jvdecimalPlaces == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_decimalPlaces{
                "Missing required key decimal_places"};
        return boost::system::error_code{129,
                                         error_missing_field_decimalPlaces};
    }

    const auto decimalPlaces = boost::json::try_value_to<int>(*jvdecimalPlaces);

    if (decimalPlaces.has_error())
    {
        return decimalPlaces.error();
    }

    const auto *jvcurrency = root.if_contains("currency");
    if (jvcurrency == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_currency{"Missing required key currency"};
        return boost::system::error_code{129, error_missing_field_currency};
    }

    const auto currency = boost::json::try_value_to<std::string>(*jvcurrency);

    if (currency.has_error())
    {
        return currency.error();
    }

    return CharityDonationAmount{
        .value = value.value(),
        .decimalPlaces = decimalPlaces.value(),
        .currency = currency.value(),
    };
}

boost::json::result_for<CharityDonation, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<CharityDonation>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "CharityDonation must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvcharityName = root.if_contains("charity_name");
    if (jvcharityName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_charityName{
                "Missing required key charity_name"};
        return boost::system::error_code{129, error_missing_field_charityName};
    }

    const auto charityName =
        boost::json::try_value_to<std::string>(*jvcharityName);

    if (charityName.has_error())
    {
        return charityName.error();
    }

    const auto *jvamount = root.if_contains("amount");
    if (jvamount == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_amount{
            "Missing required key amount"};
        return boost::system::error_code{129, error_missing_field_amount};
    }

    const auto amount =
        boost::json::try_value_to<CharityDonationAmount>(*jvamount);

    if (amount.has_error())
    {
        return amount.error();
    }

    return CharityDonation{
        .charityName = charityName.value(),
        .amount = amount.value(),
    };
}

boost::json::result_for<BitsBadgeTier, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<BitsBadgeTier>,
    const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "BitsBadgeTier must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvtier = root.if_contains("tier");
    if (jvtier == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_tier{
            "Missing required key tier"};
        return boost::system::error_code{129, error_missing_field_tier};
    }

    const auto tier = boost::json::try_value_to<int>(*jvtier);

    if (tier.has_error())
    {
        return tier.error();
    }

    return BitsBadgeTier{
        .tier = tier.value(),
    };
}

boost::json::result_for<Message, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Message>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Message must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvtext = root.if_contains("text");
    if (jvtext == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_text{
            "Missing required key text"};
        return boost::system::error_code{129, error_missing_field_text};
    }

    const auto text = boost::json::try_value_to<std::string>(*jvtext);

    if (text.has_error())
    {
        return text.error();
    }

    const auto *jvfragments = root.if_contains("fragments");
    if (jvfragments == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_fragments{"Missing required key fragments"};
        return boost::system::error_code{129, error_missing_field_fragments};
    }
    const auto fragments = boost::json::try_value_to<std::vector<
        eventsub::payload::channel_chat_notification::v1::MessageFragment>>(
        *jvfragments);
    if (fragments.has_error())
    {
        return fragments.error();
    }

    return Message{
        .text = text.value(),
        .fragments = fragments.value(),
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

    const auto *jvchatterUserID = root.if_contains("chatter_user_id");
    if (jvchatterUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_chatterUserID{
                "Missing required key chatter_user_id"};
        return boost::system::error_code{129,
                                         error_missing_field_chatterUserID};
    }

    const auto chatterUserID =
        boost::json::try_value_to<std::string>(*jvchatterUserID);

    if (chatterUserID.has_error())
    {
        return chatterUserID.error();
    }

    const auto *jvchatterUserLogin = root.if_contains("chatter_user_login");
    if (jvchatterUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_chatterUserLogin{
                "Missing required key chatter_user_login"};
        return boost::system::error_code{129,
                                         error_missing_field_chatterUserLogin};
    }

    const auto chatterUserLogin =
        boost::json::try_value_to<std::string>(*jvchatterUserLogin);

    if (chatterUserLogin.has_error())
    {
        return chatterUserLogin.error();
    }

    const auto *jvchatterUserName = root.if_contains("chatter_user_name");
    if (jvchatterUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_chatterUserName{
                "Missing required key chatter_user_name"};
        return boost::system::error_code{129,
                                         error_missing_field_chatterUserName};
    }

    const auto chatterUserName =
        boost::json::try_value_to<std::string>(*jvchatterUserName);

    if (chatterUserName.has_error())
    {
        return chatterUserName.error();
    }

    const auto *jvchatterIsAnonymous = root.if_contains("chatter_is_anonymous");
    if (jvchatterIsAnonymous == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_chatterIsAnonymous{
                "Missing required key chatter_is_anonymous"};
        return boost::system::error_code{
            129, error_missing_field_chatterIsAnonymous};
    }

    const auto chatterIsAnonymous =
        boost::json::try_value_to<bool>(*jvchatterIsAnonymous);

    if (chatterIsAnonymous.has_error())
    {
        return chatterIsAnonymous.error();
    }

    const auto *jvcolor = root.if_contains("color");
    if (jvcolor == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_color{
            "Missing required key color"};
        return boost::system::error_code{129, error_missing_field_color};
    }

    const auto color = boost::json::try_value_to<std::string>(*jvcolor);

    if (color.has_error())
    {
        return color.error();
    }

    const auto *jvbadges = root.if_contains("badges");
    if (jvbadges == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_badges{
            "Missing required key badges"};
        return boost::system::error_code{129, error_missing_field_badges};
    }
    const auto badges = boost::json::try_value_to<
        std::vector<eventsub::payload::channel_chat_notification::v1::Badge>>(
        *jvbadges);
    if (badges.has_error())
    {
        return badges.error();
    }

    const auto *jvsystemMessage = root.if_contains("system_message");
    if (jvsystemMessage == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_systemMessage{
                "Missing required key system_message"};
        return boost::system::error_code{129,
                                         error_missing_field_systemMessage};
    }

    const auto systemMessage =
        boost::json::try_value_to<std::string>(*jvsystemMessage);

    if (systemMessage.has_error())
    {
        return systemMessage.error();
    }

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

    const auto *jvmessage = root.if_contains("message");
    if (jvmessage == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_message{"Missing required key message"};
        return boost::system::error_code{129, error_missing_field_message};
    }

    const auto message = boost::json::try_value_to<Message>(*jvmessage);

    if (message.has_error())
    {
        return message.error();
    }

    const auto *jvnoticeType = root.if_contains("notice_type");
    if (jvnoticeType == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_noticeType{"Missing required key notice_type"};
        return boost::system::error_code{129, error_missing_field_noticeType};
    }

    const auto noticeType =
        boost::json::try_value_to<std::string>(*jvnoticeType);

    if (noticeType.has_error())
    {
        return noticeType.error();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Subcription>
        sub = std::nullopt;
    const auto *jvsub = root.if_contains("sub");
    if (jvsub != nullptr && !jvsub->is_null())
    {
        const auto tsub = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Subcription>(
            *jvsub);

        if (tsub.has_error())
        {
            return tsub.error();
        }
        sub = tsub.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::Resubscription>
        resub = std::nullopt;
    const auto *jvresub = root.if_contains("resub");
    if (jvresub != nullptr && !jvresub->is_null())
    {
        const auto tresub = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Resubscription>(
            *jvresub);

        if (tresub.has_error())
        {
            return tresub.error();
        }
        resub = tresub.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::GiftSubscription>
        subGift = std::nullopt;
    const auto *jvsubGift = root.if_contains("sub_gift");
    if (jvsubGift != nullptr && !jvsubGift->is_null())
    {
        const auto tsubGift = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::GiftSubscription>(
            *jvsubGift);

        if (tsubGift.has_error())
        {
            return tsubGift.error();
        }
        subGift = tsubGift.value();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::
                      CommunityGiftSubscription>
        communitySubGift = std::nullopt;
    const auto *jvcommunitySubGift = root.if_contains("community_sub_gift");
    if (jvcommunitySubGift != nullptr && !jvcommunitySubGift->is_null())
    {
        const auto tcommunitySubGift = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::
                CommunityGiftSubscription>(*jvcommunitySubGift);

        if (tcommunitySubGift.has_error())
        {
            return tcommunitySubGift.error();
        }
        communitySubGift = tcommunitySubGift.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::GiftPaidUpgrade>
        giftPaidUpgrade = std::nullopt;
    const auto *jvgiftPaidUpgrade = root.if_contains("gift_paid_upgrade");
    if (jvgiftPaidUpgrade != nullptr && !jvgiftPaidUpgrade->is_null())
    {
        const auto tgiftPaidUpgrade = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::GiftPaidUpgrade>(
            *jvgiftPaidUpgrade);

        if (tgiftPaidUpgrade.has_error())
        {
            return tgiftPaidUpgrade.error();
        }
        giftPaidUpgrade = tgiftPaidUpgrade.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::PrimePaidUpgrade>
        primePaidUpgrade = std::nullopt;
    const auto *jvprimePaidUpgrade = root.if_contains("prime_paid_upgrade");
    if (jvprimePaidUpgrade != nullptr && !jvprimePaidUpgrade->is_null())
    {
        const auto tprimePaidUpgrade = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::PrimePaidUpgrade>(
            *jvprimePaidUpgrade);

        if (tprimePaidUpgrade.has_error())
        {
            return tprimePaidUpgrade.error();
        }
        primePaidUpgrade = tprimePaidUpgrade.value();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Raid> raid =
        std::nullopt;
    const auto *jvraid = root.if_contains("raid");
    if (jvraid != nullptr && !jvraid->is_null())
    {
        const auto traid = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Raid>(*jvraid);

        if (traid.has_error())
        {
            return traid.error();
        }
        raid = traid.value();
    }

    std::optional<eventsub::payload::channel_chat_notification::v1::Unraid>
        unraid = std::nullopt;
    const auto *jvunraid = root.if_contains("unraid");
    if (jvunraid != nullptr && !jvunraid->is_null())
    {
        const auto tunraid = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Unraid>(
            *jvunraid);

        if (tunraid.has_error())
        {
            return tunraid.error();
        }
        unraid = tunraid.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::PayItForward>
        payItForward = std::nullopt;
    const auto *jvpayItForward = root.if_contains("pay_it_forward");
    if (jvpayItForward != nullptr && !jvpayItForward->is_null())
    {
        const auto tpayItForward = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::PayItForward>(
            *jvpayItForward);

        if (tpayItForward.has_error())
        {
            return tpayItForward.error();
        }
        payItForward = tpayItForward.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::Announcement>
        announcement = std::nullopt;
    const auto *jvannouncement = root.if_contains("announcement");
    if (jvannouncement != nullptr && !jvannouncement->is_null())
    {
        const auto tannouncement = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::Announcement>(
            *jvannouncement);

        if (tannouncement.has_error())
        {
            return tannouncement.error();
        }
        announcement = tannouncement.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::CharityDonation>
        charityDonation = std::nullopt;
    const auto *jvcharityDonation = root.if_contains("charity_donation");
    if (jvcharityDonation != nullptr && !jvcharityDonation->is_null())
    {
        const auto tcharityDonation = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::CharityDonation>(
            *jvcharityDonation);

        if (tcharityDonation.has_error())
        {
            return tcharityDonation.error();
        }
        charityDonation = tcharityDonation.value();
    }

    std::optional<
        eventsub::payload::channel_chat_notification::v1::BitsBadgeTier>
        bitsBadgeTier = std::nullopt;
    const auto *jvbitsBadgeTier = root.if_contains("bits_badge_tier");
    if (jvbitsBadgeTier != nullptr && !jvbitsBadgeTier->is_null())
    {
        const auto tbitsBadgeTier = boost::json::try_value_to<
            eventsub::payload::channel_chat_notification::v1::BitsBadgeTier>(
            *jvbitsBadgeTier);

        if (tbitsBadgeTier.has_error())
        {
            return tbitsBadgeTier.error();
        }
        bitsBadgeTier = tbitsBadgeTier.value();
    }

    return Event{
        .broadcasterUserID = broadcasterUserID.value(),
        .broadcasterUserLogin = broadcasterUserLogin.value(),
        .broadcasterUserName = broadcasterUserName.value(),
        .chatterUserID = chatterUserID.value(),
        .chatterUserLogin = chatterUserLogin.value(),
        .chatterUserName = chatterUserName.value(),
        .chatterIsAnonymous = chatterIsAnonymous.value(),
        .color = color.value(),
        .badges = badges.value(),
        .systemMessage = systemMessage.value(),
        .messageID = messageID.value(),
        .message = message.value(),
        .noticeType = noticeType.value(),
        .sub = sub,
        .resub = resub,
        .subGift = subGift,
        .communitySubGift = communitySubGift,
        .giftPaidUpgrade = giftPaidUpgrade,
        .primePaidUpgrade = primePaidUpgrade,
        .raid = raid,
        .unraid = unraid,
        .payItForward = payItForward,
        .announcement = announcement,
        .charityDonation = charityDonation,
        .bitsBadgeTier = bitsBadgeTier,
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

}  // namespace eventsub::payload::channel_chat_notification::v1
