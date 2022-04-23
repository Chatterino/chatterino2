#pragma once

#include <QJsonObject>
#include <QString>

#include <magic_enum.hpp>

namespace chatterino {

struct PubSubChatModeratorActionMessage {
    enum class Type {
        ModerationAction,
        ChannelTermsAction,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    PubSubChatModeratorActionMessage(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubChatModeratorActionMessage::Type>(
    chatterino::PubSubChatModeratorActionMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubChatModeratorActionMessage::Type::
            ModerationAction:
            return "moderation_action";

        case chatterino::PubSubChatModeratorActionMessage::Type::
            ChannelTermsAction:
            return "channel_terms_action";

        default:
            return default_tag;
    }
}
