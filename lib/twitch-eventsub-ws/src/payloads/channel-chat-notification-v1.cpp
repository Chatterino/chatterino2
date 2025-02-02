#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_chat_notification::v1 {

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

    auto setID = boost::json::try_value_to<std::string>(*jvsetID);

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

    auto id = boost::json::try_value_to<std::string>(*jvid);

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

    auto info = boost::json::try_value_to<std::string>(*jvinfo);

    if (info.has_error())
    {
        return info.error();
    }

    return Badge{
        .setID = std::move(setID.value()),
        .id = std::move(id.value()),
        .info = std::move(info.value()),
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

    auto prefix = boost::json::try_value_to<std::string>(*jvprefix);

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

    auto bits = boost::json::try_value_to<int>(*jvbits);

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

    auto tier = boost::json::try_value_to<int>(*jvtier);

    if (tier.has_error())
    {
        return tier.error();
    }

    return Cheermote{
        .prefix = std::move(prefix.value()),
        .bits = std::move(bits.value()),
        .tier = std::move(tier.value()),
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

    auto id = boost::json::try_value_to<std::string>(*jvid);

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

    auto emoteSetID = boost::json::try_value_to<std::string>(*jvemoteSetID);

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

    auto ownerID = boost::json::try_value_to<std::string>(*jvownerID);

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
        .id = std::move(id.value()),
        .emoteSetID = std::move(emoteSetID.value()),
        .ownerID = std::move(ownerID.value()),
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

    auto userID = boost::json::try_value_to<std::string>(*jvuserID);

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

    auto userName = boost::json::try_value_to<std::string>(*jvuserName);

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

    auto userLogin = boost::json::try_value_to<std::string>(*jvuserLogin);

    if (userLogin.has_error())
    {
        return userLogin.error();
    }

    return Mention{
        .userID = std::move(userID.value()),
        .userName = std::move(userName.value()),
        .userLogin = std::move(userLogin.value()),
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

    auto type = boost::json::try_value_to<std::string>(*jvtype);

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

    auto text = boost::json::try_value_to<std::string>(*jvtext);

    if (text.has_error())
    {
        return text.error();
    }

    std::optional<Cheermote> cheermote = std::nullopt;
    const auto *jvcheermote = root.if_contains("cheermote");
    if (jvcheermote != nullptr && !jvcheermote->is_null())
    {
        auto tcheermote = boost::json::try_value_to<Cheermote>(*jvcheermote);

        if (tcheermote.has_error())
        {
            return tcheermote.error();
        }
        cheermote = std::move(tcheermote.value());
    }

    std::optional<Emote> emote = std::nullopt;
    const auto *jvemote = root.if_contains("emote");
    if (jvemote != nullptr && !jvemote->is_null())
    {
        auto temote = boost::json::try_value_to<Emote>(*jvemote);

        if (temote.has_error())
        {
            return temote.error();
        }
        emote = std::move(temote.value());
    }

    std::optional<Mention> mention = std::nullopt;
    const auto *jvmention = root.if_contains("mention");
    if (jvmention != nullptr && !jvmention->is_null())
    {
        auto tmention = boost::json::try_value_to<Mention>(*jvmention);

        if (tmention.has_error())
        {
            return tmention.error();
        }
        mention = std::move(tmention.value());
    }

    return MessageFragment{
        .type = std::move(type.value()),
        .text = std::move(text.value()),
        .cheermote = std::move(cheermote),
        .emote = std::move(emote),
        .mention = std::move(mention),
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

    auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

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

    auto isPrime = boost::json::try_value_to<bool>(*jvisPrime);

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

    auto durationMonths = boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    return Subcription{
        .subTier = std::move(subTier.value()),
        .isPrime = std::move(isPrime.value()),
        .durationMonths = std::move(durationMonths.value()),
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

    auto cumulativeMonths = boost::json::try_value_to<int>(*jvcumulativeMonths);

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

    auto durationMonths = boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    std::optional<int> streakMonths = std::nullopt;
    const auto *jvstreakMonths = root.if_contains("streak_months");
    if (jvstreakMonths != nullptr && !jvstreakMonths->is_null())
    {
        auto tstreakMonths = boost::json::try_value_to<int>(*jvstreakMonths);

        if (tstreakMonths.has_error())
        {
            return tstreakMonths.error();
        }
        streakMonths = std::move(tstreakMonths.value());
    }

    const auto *jvsubTier = root.if_contains("sub_tier");
    if (jvsubTier == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_subTier{"Missing required key sub_tier"};
        return boost::system::error_code{129, error_missing_field_subTier};
    }

    auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

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

    auto isPrime = boost::json::try_value_to<bool>(*jvisPrime);

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

    auto isGift = boost::json::try_value_to<bool>(*jvisGift);

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

    auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = std::move(tgifterUserID.value());
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = std::move(tgifterUserName.value());
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = std::move(tgifterUserLogin.value());
    }

    return Resubscription{
        .cumulativeMonths = std::move(cumulativeMonths.value()),
        .durationMonths = std::move(durationMonths.value()),
        .streakMonths = std::move(streakMonths),
        .subTier = std::move(subTier.value()),
        .isPrime = std::move(isPrime.value()),
        .isGift = std::move(isGift.value()),
        .gifterIsAnonymous = std::move(gifterIsAnonymous.value()),
        .gifterUserID = std::move(gifterUserID),
        .gifterUserName = std::move(gifterUserName),
        .gifterUserLogin = std::move(gifterUserLogin),
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

    auto durationMonths = boost::json::try_value_to<int>(*jvdurationMonths);

    if (durationMonths.has_error())
    {
        return durationMonths.error();
    }

    std::optional<int> cumulativeTotal = std::nullopt;
    const auto *jvcumulativeTotal = root.if_contains("cumulative_total");
    if (jvcumulativeTotal != nullptr && !jvcumulativeTotal->is_null())
    {
        auto tcumulativeTotal =
            boost::json::try_value_to<int>(*jvcumulativeTotal);

        if (tcumulativeTotal.has_error())
        {
            return tcumulativeTotal.error();
        }
        cumulativeTotal = std::move(tcumulativeTotal.value());
    }

    std::optional<int> streakMonths = std::nullopt;
    const auto *jvstreakMonths = root.if_contains("streak_months");
    if (jvstreakMonths != nullptr && !jvstreakMonths->is_null())
    {
        auto tstreakMonths = boost::json::try_value_to<int>(*jvstreakMonths);

        if (tstreakMonths.has_error())
        {
            return tstreakMonths.error();
        }
        streakMonths = std::move(tstreakMonths.value());
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

    auto recipientUserID =
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

    auto recipientUserName =
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

    auto recipientUserLogin =
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

    auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    std::optional<std::string> communityGiftID = std::nullopt;
    const auto *jvcommunityGiftID = root.if_contains("community_gift_id");
    if (jvcommunityGiftID != nullptr && !jvcommunityGiftID->is_null())
    {
        auto tcommunityGiftID =
            boost::json::try_value_to<std::string>(*jvcommunityGiftID);

        if (tcommunityGiftID.has_error())
        {
            return tcommunityGiftID.error();
        }
        communityGiftID = std::move(tcommunityGiftID.value());
    }

    return GiftSubscription{
        .durationMonths = std::move(durationMonths.value()),
        .cumulativeTotal = std::move(cumulativeTotal),
        .streakMonths = std::move(streakMonths),
        .recipientUserID = std::move(recipientUserID.value()),
        .recipientUserName = std::move(recipientUserName.value()),
        .recipientUserLogin = std::move(recipientUserLogin.value()),
        .subTier = std::move(subTier.value()),
        .communityGiftID = std::move(communityGiftID),
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

    auto id = boost::json::try_value_to<std::string>(*jvid);

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

    auto total = boost::json::try_value_to<int>(*jvtotal);

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

    auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    std::optional<int> cumulativeTotal = std::nullopt;
    const auto *jvcumulativeTotal = root.if_contains("cumulative_total");
    if (jvcumulativeTotal != nullptr && !jvcumulativeTotal->is_null())
    {
        auto tcumulativeTotal =
            boost::json::try_value_to<int>(*jvcumulativeTotal);

        if (tcumulativeTotal.has_error())
        {
            return tcumulativeTotal.error();
        }
        cumulativeTotal = std::move(tcumulativeTotal.value());
    }

    return CommunityGiftSubscription{
        .id = std::move(id.value()),
        .total = std::move(total.value()),
        .subTier = std::move(subTier.value()),
        .cumulativeTotal = std::move(cumulativeTotal),
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

    auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = std::move(tgifterUserID.value());
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = std::move(tgifterUserName.value());
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = std::move(tgifterUserLogin.value());
    }

    return GiftPaidUpgrade{
        .gifterIsAnonymous = std::move(gifterIsAnonymous.value()),
        .gifterUserID = std::move(gifterUserID),
        .gifterUserName = std::move(gifterUserName),
        .gifterUserLogin = std::move(gifterUserLogin),
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

    auto subTier = boost::json::try_value_to<std::string>(*jvsubTier);

    if (subTier.has_error())
    {
        return subTier.error();
    }

    return PrimePaidUpgrade{
        .subTier = std::move(subTier.value()),
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

    const auto *jvprofileImageURL = root.if_contains("profile_image_url");
    if (jvprofileImageURL == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_profileImageURL{
                "Missing required key profile_image_url"};
        return boost::system::error_code{129,
                                         error_missing_field_profileImageURL};
    }

    auto profileImageURL =
        boost::json::try_value_to<std::string>(*jvprofileImageURL);

    if (profileImageURL.has_error())
    {
        return profileImageURL.error();
    }

    return Raid{
        .userID = std::move(userID.value()),
        .userName = std::move(userName.value()),
        .userLogin = std::move(userLogin.value()),
        .viewerCount = std::move(viewerCount.value()),
        .profileImageURL = std::move(profileImageURL.value()),
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

    auto gifterIsAnonymous =
        boost::json::try_value_to<bool>(*jvgifterIsAnonymous);

    if (gifterIsAnonymous.has_error())
    {
        return gifterIsAnonymous.error();
    }

    std::optional<std::string> gifterUserID = std::nullopt;
    const auto *jvgifterUserID = root.if_contains("gifter_user_id");
    if (jvgifterUserID != nullptr && !jvgifterUserID->is_null())
    {
        auto tgifterUserID =
            boost::json::try_value_to<std::string>(*jvgifterUserID);

        if (tgifterUserID.has_error())
        {
            return tgifterUserID.error();
        }
        gifterUserID = std::move(tgifterUserID.value());
    }

    std::optional<std::string> gifterUserName = std::nullopt;
    const auto *jvgifterUserName = root.if_contains("gifter_user_name");
    if (jvgifterUserName != nullptr && !jvgifterUserName->is_null())
    {
        auto tgifterUserName =
            boost::json::try_value_to<std::string>(*jvgifterUserName);

        if (tgifterUserName.has_error())
        {
            return tgifterUserName.error();
        }
        gifterUserName = std::move(tgifterUserName.value());
    }

    std::optional<std::string> gifterUserLogin = std::nullopt;
    const auto *jvgifterUserLogin = root.if_contains("gifter_user_login");
    if (jvgifterUserLogin != nullptr && !jvgifterUserLogin->is_null())
    {
        auto tgifterUserLogin =
            boost::json::try_value_to<std::string>(*jvgifterUserLogin);

        if (tgifterUserLogin.has_error())
        {
            return tgifterUserLogin.error();
        }
        gifterUserLogin = std::move(tgifterUserLogin.value());
    }

    return PayItForward{
        .gifterIsAnonymous = std::move(gifterIsAnonymous.value()),
        .gifterUserID = std::move(gifterUserID),
        .gifterUserName = std::move(gifterUserName),
        .gifterUserLogin = std::move(gifterUserLogin),
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

    auto color = boost::json::try_value_to<std::string>(*jvcolor);

    if (color.has_error())
    {
        return color.error();
    }

    return Announcement{
        .color = std::move(color.value()),
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

    auto value = boost::json::try_value_to<int>(*jvvalue);

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

    auto decimalPlaces = boost::json::try_value_to<int>(*jvdecimalPlaces);

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

    auto currency = boost::json::try_value_to<std::string>(*jvcurrency);

    if (currency.has_error())
    {
        return currency.error();
    }

    return CharityDonationAmount{
        .value = std::move(value.value()),
        .decimalPlaces = std::move(decimalPlaces.value()),
        .currency = std::move(currency.value()),
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

    auto charityName = boost::json::try_value_to<std::string>(*jvcharityName);

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

    auto amount = boost::json::try_value_to<CharityDonationAmount>(*jvamount);

    if (amount.has_error())
    {
        return amount.error();
    }

    return CharityDonation{
        .charityName = std::move(charityName.value()),
        .amount = std::move(amount.value()),
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

    auto tier = boost::json::try_value_to<int>(*jvtier);

    if (tier.has_error())
    {
        return tier.error();
    }

    return BitsBadgeTier{
        .tier = std::move(tier.value()),
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

    auto text = boost::json::try_value_to<std::string>(*jvtext);

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
    const auto fragments =
        boost::json::try_value_to<std::vector<MessageFragment>>(*jvfragments);
    if (fragments.has_error())
    {
        return fragments.error();
    }

    return Message{
        .text = std::move(text.value()),
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

    auto broadcasterUserID =
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

    auto broadcasterUserLogin =
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

    auto broadcasterUserName =
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

    auto chatterUserID =
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

    auto chatterUserLogin =
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

    auto chatterUserName =
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

    auto chatterIsAnonymous =
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

    auto color = boost::json::try_value_to<std::string>(*jvcolor);

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
    const auto badges =
        boost::json::try_value_to<std::vector<Badge>>(*jvbadges);
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

    auto systemMessage =
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

    auto messageID = boost::json::try_value_to<std::string>(*jvmessageID);

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

    auto message = boost::json::try_value_to<Message>(*jvmessage);

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

    auto noticeType = boost::json::try_value_to<std::string>(*jvnoticeType);

    if (noticeType.has_error())
    {
        return noticeType.error();
    }

    std::optional<Subcription> sub = std::nullopt;
    const auto *jvsub = root.if_contains("sub");
    if (jvsub != nullptr && !jvsub->is_null())
    {
        auto tsub = boost::json::try_value_to<Subcription>(*jvsub);

        if (tsub.has_error())
        {
            return tsub.error();
        }
        sub = std::move(tsub.value());
    }

    std::optional<Resubscription> resub = std::nullopt;
    const auto *jvresub = root.if_contains("resub");
    if (jvresub != nullptr && !jvresub->is_null())
    {
        auto tresub = boost::json::try_value_to<Resubscription>(*jvresub);

        if (tresub.has_error())
        {
            return tresub.error();
        }
        resub = std::move(tresub.value());
    }

    std::optional<GiftSubscription> subGift = std::nullopt;
    const auto *jvsubGift = root.if_contains("sub_gift");
    if (jvsubGift != nullptr && !jvsubGift->is_null())
    {
        auto tsubGift = boost::json::try_value_to<GiftSubscription>(*jvsubGift);

        if (tsubGift.has_error())
        {
            return tsubGift.error();
        }
        subGift = std::move(tsubGift.value());
    }

    std::optional<CommunityGiftSubscription> communitySubGift = std::nullopt;
    const auto *jvcommunitySubGift = root.if_contains("community_sub_gift");
    if (jvcommunitySubGift != nullptr && !jvcommunitySubGift->is_null())
    {
        auto tcommunitySubGift =
            boost::json::try_value_to<CommunityGiftSubscription>(
                *jvcommunitySubGift);

        if (tcommunitySubGift.has_error())
        {
            return tcommunitySubGift.error();
        }
        communitySubGift = std::move(tcommunitySubGift.value());
    }

    std::optional<GiftPaidUpgrade> giftPaidUpgrade = std::nullopt;
    const auto *jvgiftPaidUpgrade = root.if_contains("gift_paid_upgrade");
    if (jvgiftPaidUpgrade != nullptr && !jvgiftPaidUpgrade->is_null())
    {
        auto tgiftPaidUpgrade =
            boost::json::try_value_to<GiftPaidUpgrade>(*jvgiftPaidUpgrade);

        if (tgiftPaidUpgrade.has_error())
        {
            return tgiftPaidUpgrade.error();
        }
        giftPaidUpgrade = std::move(tgiftPaidUpgrade.value());
    }

    std::optional<PrimePaidUpgrade> primePaidUpgrade = std::nullopt;
    const auto *jvprimePaidUpgrade = root.if_contains("prime_paid_upgrade");
    if (jvprimePaidUpgrade != nullptr && !jvprimePaidUpgrade->is_null())
    {
        auto tprimePaidUpgrade =
            boost::json::try_value_to<PrimePaidUpgrade>(*jvprimePaidUpgrade);

        if (tprimePaidUpgrade.has_error())
        {
            return tprimePaidUpgrade.error();
        }
        primePaidUpgrade = std::move(tprimePaidUpgrade.value());
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

    std::optional<PayItForward> payItForward = std::nullopt;
    const auto *jvpayItForward = root.if_contains("pay_it_forward");
    if (jvpayItForward != nullptr && !jvpayItForward->is_null())
    {
        auto tpayItForward =
            boost::json::try_value_to<PayItForward>(*jvpayItForward);

        if (tpayItForward.has_error())
        {
            return tpayItForward.error();
        }
        payItForward = std::move(tpayItForward.value());
    }

    std::optional<Announcement> announcement = std::nullopt;
    const auto *jvannouncement = root.if_contains("announcement");
    if (jvannouncement != nullptr && !jvannouncement->is_null())
    {
        auto tannouncement =
            boost::json::try_value_to<Announcement>(*jvannouncement);

        if (tannouncement.has_error())
        {
            return tannouncement.error();
        }
        announcement = std::move(tannouncement.value());
    }

    std::optional<CharityDonation> charityDonation = std::nullopt;
    const auto *jvcharityDonation = root.if_contains("charity_donation");
    if (jvcharityDonation != nullptr && !jvcharityDonation->is_null())
    {
        auto tcharityDonation =
            boost::json::try_value_to<CharityDonation>(*jvcharityDonation);

        if (tcharityDonation.has_error())
        {
            return tcharityDonation.error();
        }
        charityDonation = std::move(tcharityDonation.value());
    }

    std::optional<BitsBadgeTier> bitsBadgeTier = std::nullopt;
    const auto *jvbitsBadgeTier = root.if_contains("bits_badge_tier");
    if (jvbitsBadgeTier != nullptr && !jvbitsBadgeTier->is_null())
    {
        auto tbitsBadgeTier =
            boost::json::try_value_to<BitsBadgeTier>(*jvbitsBadgeTier);

        if (tbitsBadgeTier.has_error())
        {
            return tbitsBadgeTier.error();
        }
        bitsBadgeTier = std::move(tbitsBadgeTier.value());
    }

    return Event{
        .broadcasterUserID = std::move(broadcasterUserID.value()),
        .broadcasterUserLogin = std::move(broadcasterUserLogin.value()),
        .broadcasterUserName = std::move(broadcasterUserName.value()),
        .chatterUserID = std::move(chatterUserID.value()),
        .chatterUserLogin = std::move(chatterUserLogin.value()),
        .chatterUserName = std::move(chatterUserName.value()),
        .chatterIsAnonymous = std::move(chatterIsAnonymous.value()),
        .color = std::move(color.value()),
        .badges = badges.value(),
        .systemMessage = std::move(systemMessage.value()),
        .messageID = std::move(messageID.value()),
        .message = std::move(message.value()),
        .noticeType = std::move(noticeType.value()),
        .sub = std::move(sub),
        .resub = std::move(resub),
        .subGift = std::move(subGift),
        .communitySubGift = std::move(communitySubGift),
        .giftPaidUpgrade = std::move(giftPaidUpgrade),
        .primePaidUpgrade = std::move(primePaidUpgrade),
        .raid = std::move(raid),
        .unraid = std::move(unraid),
        .payItForward = std::move(payItForward),
        .announcement = std::move(announcement),
        .charityDonation = std::move(charityDonation),
        .bitsBadgeTier = std::move(bitsBadgeTier),
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

}  // namespace chatterino::eventsub::lib::payload::channel_chat_notification::v1
