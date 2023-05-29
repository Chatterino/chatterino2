#pragma once

#include <magic_enum.hpp>
#include <QColor>
#include <QJsonObject>
#include <QString>

namespace chatterino {

struct PubSubLowTrustUserMessage {
    enum class Type {
        NewMessage,
        TreatmentUpdate,

        INVALID,
    };
    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    // This here is documented to be always available, but structure may differ
    QJsonObject types;
    QString banEvasionEvaluation;
    QString treatment;
    QString lowTrustID;
    QString channelID;

    QString updaterUserID;
    QString updaterLogin;
    QString updaterDisplayName;

    // Treatment Update
    QString targetUserID;
    QString targetUsername;

    // New message
    QString senderUserID;
    QString senderUserLogin;
    QString senderDisplayName;

    QJsonObject messageContent;
    QString messageID;

    PubSubLowTrustUserMessage(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<
    chatterino::PubSubLowTrustUserMessage::Type>(
    chatterino::PubSubLowTrustUserMessage::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubLowTrustUserMessage::Type::NewMessage:
            return "low_trust_user_new_message";

        case chatterino::PubSubLowTrustUserMessage::Type::TreatmentUpdate:
            return "low_trust_user_treatment_update";

        default:
            return default_tag;
    }
}