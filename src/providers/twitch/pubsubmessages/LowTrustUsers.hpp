#pragma once

#include <common/FlagsEnum.hpp>
#include <magic_enum/magic_enum.hpp>
#include <QColor>
#include <QJsonObject>
#include <QString>

namespace chatterino {

struct LowTrustUserChatBadge {
    QString id;
    QString version;

    explicit LowTrustUserChatBadge(const QJsonObject &obj)
        : id(obj.value("id").toString())
        , version(obj.value("version").toString())
    {
    }
};

struct PubSubLowTrustUsersMessage {
    enum class Type {
        UserMessage,
        TreatmentUpdate,

        INVALID,
    };
    QString typeString;
    Type type = Type::INVALID;

    enum class Treatment {
        NO_TREATMENT,
        ACTIVE_MONITORING,
        RESTRICTED,

        INVALID,
    };
    QString treatmentString;
    Treatment treatment = Treatment::INVALID;

    enum class EvasionEvaluation {
        UNKNOWN_EVADER,
        UNLIKELY_EVADER,
        LIKELY_EVADER,
        POSSIBLE_EVADER,

        INVALID,
    };
    QString evasionString;
    EvasionEvaluation evasionEvaluation = EvasionEvaluation::INVALID;

    enum class RestrictionType : uint8_t {
        UNKNOWN_TYPE = 1 << 0,
        MANUALLY_ADDED = 1 << 1,
        DETECTED_BAN_EVADER = 1 << 2,
        BANNED_IN_SHARED_CHANNEL = 1 << 3,

        INVALID = 1 << 4,
    };
    FlagsEnum<RestrictionType> restrictionTypes;

    // QString lowTrustID; // unused, more relevant for first-party
    QString channelID;
    QString suspiciousUserID;
    QString suspiciousUserLogin;
    QString suspiciousUserDisplayName;
    QString updatedByUserId;
    QString updatedByUserLogin;
    QString updatedByUserDisplayName;
    QString evaluatedAt;
    QString updatedAt;
    QString formattedUpdatedAt;

    /*
     * Fields that are only populated on new message.
     * i.e., not the treatment update event type.
    */
    QString text;
    QString msgID;
    QString sentAt;
    QColor suspiciousUserColor;  // undocumented
    std::vector<QString> sharedBanChannelIDs;
    std::vector<LowTrustUserChatBadge> senderBadges;  // undocumented

    PubSubLowTrustUsersMessage() = default;
    explicit PubSubLowTrustUsersMessage(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubLowTrustUsersMessage::Type>(
    chatterino::PubSubLowTrustUsersMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubLowTrustUsersMessage::Type::UserMessage:
            return "low_trust_user_new_message";

        case chatterino::PubSubLowTrustUsersMessage::Type::TreatmentUpdate:
            return "low_trust_user_treatment_update";

        default:
            return default_tag;
    }
}
