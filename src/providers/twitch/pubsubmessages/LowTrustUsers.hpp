#pragma once

#include <common/FlagsEnum.hpp>
#include <magic_enum/magic_enum.hpp>
#include <QColor>
#include <QDateTime>
#include <QJsonArray>
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
    /**
     * The type of this message
     */
    enum class Type {
        /**
         * An incoming message from someone marked as low trust
         */
        UserMessage,

        /**
         * An incoming update about a user's low trust status
         */
        TreatmentUpdate,

        INVALID,
    };

    enum class Treatment {
        NoTreatment,
        ActiveMonitoring,
        Restricted,

        INVALID,
    };

    enum class EvasionEvaluation {
        UNKNOWN_EVADER,
        UNLIKELY_EVADER,
        LIKELY_EVADER,
        POSSIBLE_EVADER,

        INVALID,
    };

    enum class RestrictionType : uint8_t {
        UNKNOWN_TYPE = 1 << 0,
        MANUALLY_ADDED = 1 << 1,
        DETECTED_BAN_EVADER = 1 << 2,
        BANNED_IN_SHARED_CHANNEL = 1 << 3,

        INVALID = 1 << 4,
    };

    QString typeString;
    Type type = Type::INVALID;

    QString treatmentString;
    Treatment treatment = Treatment::INVALID;

    QString evasionString;
    EvasionEvaluation evasionEvaluation = EvasionEvaluation::INVALID;

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

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubLowTrustUsersMessage::Treatment>(
    chatterino::PubSubLowTrustUsersMessage::Treatment value) noexcept
{
    using Treatment = chatterino::PubSubLowTrustUsersMessage::Treatment;
    switch (value)
    {
        case Treatment::NoTreatment:
            return "NO_TREATMENT";

        case Treatment::ActiveMonitoring:
            return "ACTIVE_MONITORING";

        case Treatment::Restricted:
            return "RESTRICTED";

        default:
            return default_tag;
    }
}
