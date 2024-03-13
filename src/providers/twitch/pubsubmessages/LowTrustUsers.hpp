#pragma once

#include "providers/twitch/TwitchBadge.hpp"

#include <common/FlagsEnum.hpp>
#include <magic_enum/magic_enum.hpp>
#include <QColor>
#include <QJsonObject>
#include <QString>

namespace chatterino {

struct PubSubLowTrustUsersMessage {
    struct Fragment {
        QString text;
        QString emoteID;

        explicit Fragment(const QJsonObject &obj)
            : text(obj.value("text").toString())
            , emoteID(obj.value("emoticon")
                          .toObject()
                          .value("emoticonID")
                          .toString())
        {
        }
    };

    /**
     * The type of low trust message update
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

    /**
     * The treatment set for the suspicious user
     */
    enum class Treatment {
        NoTreatment,
        ActiveMonitoring,
        Restricted,

        INVALID,
    };

    /**
     * A ban evasion likelihood value (if any) that has been applied to the user
     * automatically by Twitch
     */
    enum class EvasionEvaluation {
        UnknownEvader,
        UnlikelyEvader,
        LikelyEvader,
        PossibleEvader,

        INVALID,
    };

    /**
     * Restriction type (if any) that apply to the suspicious user
     */
    enum class RestrictionType : uint8_t {
        UnknownType = 1 << 0,
        ManuallyAdded = 1 << 1,
        DetectedBanEvader = 1 << 2,
        BannedInSharedChannel = 1 << 3,

        INVALID = 1 << 4,
    };

    Type type = Type::INVALID;

    Treatment treatment = Treatment::INVALID;

    EvasionEvaluation evasionEvaluation = EvasionEvaluation::INVALID;

    FlagsEnum<RestrictionType> restrictionTypes;

    QString channelID;

    QString suspiciousUserID;
    QString suspiciousUserLogin;
    QString suspiciousUserDisplayName;

    QString updatedByUserID;
    QString updatedByUserLogin;
    QString updatedByUserDisplayName;

    /**
     * Formatted timestamp of when the treatment was last updated for the suspicious user
     */
    QString updatedAt;

    /**
     * Plain text of the message sent.
     * Only used for the UserMessage type.
     */
    QString text;

    /**
     * Pre-parsed components of the message.
     * Only used for the UserMessage type.
     */
    std::vector<Fragment> fragments;

    /**
     * ID of the message.
     * Only used for the UserMessage type.
     */
    QString msgID;

    /**
     * RFC3339 timestamp of when the message was sent.
     * Only used for the UserMessage type.
     */
    QString sentAt;

    /**
     * Color of the user who sent the message.
     * Only used for the UserMessage type.
     */
    QColor suspiciousUserColor;

    /**
     * A list of channel IDs where the suspicious user is also banned.
     * Only used for the UserMessage type.
     */
    std::vector<QString> sharedBanChannelIDs;

    /**
     * A list of badges of the user who sent the message.
     * Only used for the UserMessage type.
     */
    std::vector<Badge> senderBadges;

    /**
     * Stores the string value of `type`
     * Useful in case type shows up as invalid after being parsed
     */
    QString typeString;

    /**
     * Stores the string value of `treatment`
     * Useful in case treatment shows up as invalid after being parsed
     */
    QString treatmentString;

    /**
     * Stores the string value of `ban_evasion_evaluation`
     * Useful in case evasionEvaluation shows up as invalid after being parsed
     */
    QString evasionEvaluationString;

    /**
     * Stores the string value of `updated_at`
     * Useful in case formattedUpdatedAt doesn't parse correctly
     */
    QString updatedAtString;

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

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubLowTrustUsersMessage::EvasionEvaluation>(
    chatterino::PubSubLowTrustUsersMessage::EvasionEvaluation value) noexcept
{
    using EvasionEvaluation =
        chatterino::PubSubLowTrustUsersMessage::EvasionEvaluation;
    switch (value)
    {
        case EvasionEvaluation::UnknownEvader:
            return "UNKNOWN_EVADER";

        case EvasionEvaluation::UnlikelyEvader:
            return "UNLIKELY_EVADER";

        case EvasionEvaluation::LikelyEvader:
            return "LIKELY_EVADER";

        case EvasionEvaluation::PossibleEvader:
            return "POSSIBLE_EVADER";

        default:
            return default_tag;
    }
}

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubLowTrustUsersMessage::RestrictionType>(
    chatterino::PubSubLowTrustUsersMessage::RestrictionType value) noexcept
{
    using RestrictionType =
        chatterino::PubSubLowTrustUsersMessage::RestrictionType;
    switch (value)
    {
        case RestrictionType::UnknownType:
            return "UNKNOWN_TYPE";

        case RestrictionType::ManuallyAdded:
            return "MANUALLY_ADDED";

        case RestrictionType::DetectedBanEvader:
            return "DETECTED_BAN_EVADER";

        case RestrictionType::BannedInSharedChannel:
            return "BANNED_IN_SHARED_CHANNEL";

        default:
            return default_tag;
    }
}
