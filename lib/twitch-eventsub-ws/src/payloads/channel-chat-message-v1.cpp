#include "twitch-eventsub-ws/payloads/channel-chat-message-v1.hpp"

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_chat_message::v1 {

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

boost::json::result_for<Cheer, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Cheer>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Cheer must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

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

    return Cheer{
        .bits = std::move(bits.value()),
    };
}

boost::json::result_for<Reply, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Reply>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        static const error::ApplicationErrorCategory errorMustBeObject{
            "Reply must be an object"};
        return boost::system::error_code{129, errorMustBeObject};
    }
    const auto &root = jvRoot.get_object();

    const auto *jvparentMessageID = root.if_contains("parent_message_id");
    if (jvparentMessageID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_parentMessageID{
                "Missing required key parent_message_id"};
        return boost::system::error_code{129,
                                         error_missing_field_parentMessageID};
    }

    auto parentMessageID =
        boost::json::try_value_to<std::string>(*jvparentMessageID);

    if (parentMessageID.has_error())
    {
        return parentMessageID.error();
    }

    const auto *jvparentUserID = root.if_contains("parent_user_id");
    if (jvparentUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_parentUserID{
                "Missing required key parent_user_id"};
        return boost::system::error_code{129, error_missing_field_parentUserID};
    }

    auto parentUserID = boost::json::try_value_to<std::string>(*jvparentUserID);

    if (parentUserID.has_error())
    {
        return parentUserID.error();
    }

    const auto *jvparentUserLogin = root.if_contains("parent_user_login");
    if (jvparentUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_parentUserLogin{
                "Missing required key parent_user_login"};
        return boost::system::error_code{129,
                                         error_missing_field_parentUserLogin};
    }

    auto parentUserLogin =
        boost::json::try_value_to<std::string>(*jvparentUserLogin);

    if (parentUserLogin.has_error())
    {
        return parentUserLogin.error();
    }

    const auto *jvparentUserName = root.if_contains("parent_user_name");
    if (jvparentUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_parentUserName{
                "Missing required key parent_user_name"};
        return boost::system::error_code{129,
                                         error_missing_field_parentUserName};
    }

    auto parentUserName =
        boost::json::try_value_to<std::string>(*jvparentUserName);

    if (parentUserName.has_error())
    {
        return parentUserName.error();
    }

    const auto *jvparentMessageBody = root.if_contains("parent_message_body");
    if (jvparentMessageBody == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_parentMessageBody{
                "Missing required key parent_message_body"};
        return boost::system::error_code{129,
                                         error_missing_field_parentMessageBody};
    }

    auto parentMessageBody =
        boost::json::try_value_to<std::string>(*jvparentMessageBody);

    if (parentMessageBody.has_error())
    {
        return parentMessageBody.error();
    }

    const auto *jvthreadMessageID = root.if_contains("thread_message_id");
    if (jvthreadMessageID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_threadMessageID{
                "Missing required key thread_message_id"};
        return boost::system::error_code{129,
                                         error_missing_field_threadMessageID};
    }

    auto threadMessageID =
        boost::json::try_value_to<std::string>(*jvthreadMessageID);

    if (threadMessageID.has_error())
    {
        return threadMessageID.error();
    }

    const auto *jvthreadUserID = root.if_contains("thread_user_id");
    if (jvthreadUserID == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_threadUserID{
                "Missing required key thread_user_id"};
        return boost::system::error_code{129, error_missing_field_threadUserID};
    }

    auto threadUserID = boost::json::try_value_to<std::string>(*jvthreadUserID);

    if (threadUserID.has_error())
    {
        return threadUserID.error();
    }

    const auto *jvthreadUserLogin = root.if_contains("thread_user_login");
    if (jvthreadUserLogin == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_threadUserLogin{
                "Missing required key thread_user_login"};
        return boost::system::error_code{129,
                                         error_missing_field_threadUserLogin};
    }

    auto threadUserLogin =
        boost::json::try_value_to<std::string>(*jvthreadUserLogin);

    if (threadUserLogin.has_error())
    {
        return threadUserLogin.error();
    }

    const auto *jvthreadUserName = root.if_contains("thread_user_name");
    if (jvthreadUserName == nullptr)
    {
        static const error::ApplicationErrorCategory
            error_missing_field_threadUserName{
                "Missing required key thread_user_name"};
        return boost::system::error_code{129,
                                         error_missing_field_threadUserName};
    }

    auto threadUserName =
        boost::json::try_value_to<std::string>(*jvthreadUserName);

    if (threadUserName.has_error())
    {
        return threadUserName.error();
    }

    return Reply{
        .parentMessageID = std::move(parentMessageID.value()),
        .parentUserID = std::move(parentUserID.value()),
        .parentUserLogin = std::move(parentUserLogin.value()),
        .parentUserName = std::move(parentUserName.value()),
        .parentMessageBody = std::move(parentMessageBody.value()),
        .threadMessageID = std::move(threadMessageID.value()),
        .threadUserID = std::move(threadUserID.value()),
        .threadUserLogin = std::move(threadUserLogin.value()),
        .threadUserName = std::move(threadUserName.value()),
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

    std::optional<Cheer> cheer = std::nullopt;
    const auto *jvcheer = root.if_contains("cheer");
    if (jvcheer != nullptr && !jvcheer->is_null())
    {
        auto tcheer = boost::json::try_value_to<Cheer>(*jvcheer);

        if (tcheer.has_error())
        {
            return tcheer.error();
        }
        cheer = std::move(tcheer.value());
    }

    std::optional<Reply> reply = std::nullopt;
    const auto *jvreply = root.if_contains("reply");
    if (jvreply != nullptr && !jvreply->is_null())
    {
        auto treply = boost::json::try_value_to<Reply>(*jvreply);

        if (treply.has_error())
        {
            return treply.error();
        }
        reply = std::move(treply.value());
    }

    std::optional<std::string> channelPointsCustomRewardID = std::nullopt;
    const auto *jvchannelPointsCustomRewardID =
        root.if_contains("channel_points_custom_reward_id");
    if (jvchannelPointsCustomRewardID != nullptr &&
        !jvchannelPointsCustomRewardID->is_null())
    {
        auto tchannelPointsCustomRewardID =
            boost::json::try_value_to<std::string>(
                *jvchannelPointsCustomRewardID);

        if (tchannelPointsCustomRewardID.has_error())
        {
            return tchannelPointsCustomRewardID.error();
        }
        channelPointsCustomRewardID =
            std::move(tchannelPointsCustomRewardID.value());
    }

    return Event{
        .broadcasterUserID = std::move(broadcasterUserID.value()),
        .broadcasterUserLogin = std::move(broadcasterUserLogin.value()),
        .broadcasterUserName = std::move(broadcasterUserName.value()),
        .chatterUserID = std::move(chatterUserID.value()),
        .chatterUserLogin = std::move(chatterUserLogin.value()),
        .chatterUserName = std::move(chatterUserName.value()),
        .color = std::move(color.value()),
        .badges = badges.value(),
        .messageID = std::move(messageID.value()),
        .messageType = std::move(messageType.value()),
        .message = std::move(message.value()),
        .cheer = std::move(cheer),
        .reply = std::move(reply),
        .channelPointsCustomRewardID = std::move(channelPointsCustomRewardID),
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

}  // namespace chatterino::eventsub::lib::payload::channel_chat_message::v1
