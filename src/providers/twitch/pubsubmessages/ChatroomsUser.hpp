#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include <boost/optional/optional.hpp>
#include <magic_enum.hpp>
#include <optional>

namespace chatterino {

struct PubSubChatroomsUserMessage {
    enum class Type {
        UserModerationAction,
        ChannelBannedUpdate,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    QString action;
    QString channelID;
    boost::optional<QDateTime> expiresAt;
    int expiresInMs;
    QString reason;
    QString targetID;

    boost::optional<bool> userIsRestricted = boost::none;

    PubSubChatroomsUserMessage(const QJsonObject &root);
};
}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubChatroomsUserMessage::Type>(
    chatterino::PubSubChatroomsUserMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubChatroomsUserMessage::Type::UserModerationAction:
            return "user_moderation_action";
        case chatterino::PubSubChatroomsUserMessage::Type::ChannelBannedUpdate:
            return "channel_banned_alias_restriction_update";
        default:
            return default_tag;
    }
}
