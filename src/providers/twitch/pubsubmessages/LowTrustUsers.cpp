#include "providers/twitch/pubsubmessages/LowTrustUsers.hpp"

#include <QDateTime>
#include <QJsonArray>

namespace chatterino {

PubSubLowTrustUsersMessage::PubSubLowTrustUsersMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
{
    if (const auto oType =
            magic_enum::enum_cast<Type>(this->typeString.toStdString());
        oType.has_value())
    {
        this->type = oType.value();
    }

    auto data = root.value("data").toObject();

    if (this->type == Type::UserMessage)
    {
        this->msgID = data.value("message_id").toString();
        this->sentAt = data.value("sent_at").toString();
        this->text =
            data.value("message_content").toObject().value("text").toString();

        // the rest of the data is within a nested object
        data = data.value("low_trust_user").toObject();

        const auto sender = data.value("sender").toObject();
        this->suspiciousUserID = sender.value("user_id").toString();
        this->suspiciousUserLogin = sender.value("login").toString();
        this->suspiciousUserDisplayName =
            sender.value("display_name").toString();
        this->suspiciousUserColor =
            QColor(sender.value("chat_color").toString());

        std::vector<LowTrustUserChatBadge> badges;
        for (const auto &badge : sender.value("badges").toArray())
        {
            badges.emplace_back(badge.toObject());
        }
        this->senderBadges = badges;

        const auto sharedValue = data.value("shared_ban_channel_ids");
        std::vector<QString> sharedIDs;
        if (!sharedValue.isNull())
        {
            for (const auto &id : sharedValue.toArray())
            {
                sharedIDs.emplace_back(id.toString());
            }
        }
        this->sharedBanChannelIDs = sharedIDs;
    }
    else
    {
        this->suspiciousUserID = data.value("target_user_id").toString();
        this->suspiciousUserLogin = data.value("target_user").toString();
        this->suspiciousUserDisplayName = this->suspiciousUserLogin;
    }

    this->channelID = data.value("channel_id").toString();
    this->updatedAtString = data.value("updated_at").toString();
    this->updatedAt = QDateTime::fromString(this->updatedAtString, Qt::ISODate)
                          .toLocalTime()
                          .toString("MMM d yyyy, h:mm ap");

    const auto updatedBy = data.value("updated_by").toObject();
    this->updatedByUserID = updatedBy.value("id").toString();
    this->updatedByUserLogin = updatedBy.value("login").toString();
    this->updatedByUserDisplayName = updatedBy.value("display_name").toString();

    this->treatmentString = data.value("treatment").toString();
    if (const auto oTreatment = magic_enum::enum_cast<Treatment>(
            this->treatmentString.toStdString());
        oTreatment.has_value())
    {
        this->treatment = oTreatment.value();
    }

    this->evasionEvaluationString =
        data.value("ban_evasion_evaluation").toString();
    if (const auto oEvaluation = magic_enum::enum_cast<EvasionEvaluation>(
            this->evasionEvaluationString.toStdString());
        oEvaluation.has_value())
    {
        this->evasionEvaluation = oEvaluation.value();
    }

    FlagsEnum<RestrictionType> restrictions;
    for (const auto &rType : data.value("types").toArray())
    {
        if (const auto oRestriction = magic_enum::enum_cast<RestrictionType>(
                rType.toString().toStdString());
            oRestriction.has_value())
        {
            restrictions.set(oRestriction.value());
        }
    }
    this->restrictionTypes = restrictions;
}

}  // namespace chatterino
