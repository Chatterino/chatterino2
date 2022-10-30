#pragma once

#include <QJsonObject>
#include <QString>

#include <magic_enum.hpp>

namespace chatterino {

struct PubSubCommunityPointsChannelV1Message {
    enum class Type {
        RewardRedeemed,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    PubSubCommunityPointsChannelV1Message(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubCommunityPointsChannelV1Message::Type>(
    chatterino::PubSubCommunityPointsChannelV1Message::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubCommunityPointsChannelV1Message::Type::
            RewardRedeemed:
            return "reward-redeemed";
        default:
            return default_tag;
    }
}
