#include "providers/twitch/pubsubmessages/LowTrustUsers.hpp"

#include "util/QMagicEnum.hpp"

#include <QDateTime>
#include <QJsonArray>

namespace chatterino {

PubSubLowTrustUsersMessage::PubSubLowTrustUsersMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
{
    if (const auto oType = qmagicenum::enumCast<Type>(this->typeString);
        oType.has_value())
    {
        this->type = oType.value();
    }

    auto data = root.value("data").toObject();

    if (this->type == Type::UserMessage)
    {
        this->msgID = data.value("message_id").toString();
        this->sentAt = data.value("sent_at").toString();
        const auto content = data.value("message_content").toObject();
        this->text = content.value("text").toString();
        for (const auto &part : content.value("fragments").toArray())
        {
            this->fragments.emplace_back(part.toObject());
        }

        // the rest of the data is within a nested object
        data = data.value("low_trust_user").toObject();

        const auto sender = data.value("sender").toObject();
        this->suspiciousUserID = sender.value("user_id").toString();
        this->suspiciousUserLogin = sender.value("login").toString();
        this->suspiciousUserDisplayName =
            sender.value("display_name").toString();
        this->suspiciousUserColor =
            QColor(sender.value("chat_color").toString());

        for (const auto &badge : sender.value("badges").toArray())
        {
            const auto badgeObj = badge.toObject();
            const auto badgeID = badgeObj.value("id").toString();
            const auto badgeVersion = badgeObj.value("version").toString();
            this->senderBadges.emplace_back(Badge{badgeID, badgeVersion});
        }

        const auto sharedValue = data.value("shared_ban_channel_ids");
        if (!sharedValue.isNull())
        {
            for (const auto &id : sharedValue.toArray())
            {
                this->sharedBanChannelIDs.emplace_back(id.toString());
            }
        }
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
    if (const auto oTreatment =
            qmagicenum::enumCast<Treatment>(this->treatmentString);
        oTreatment.has_value())
    {
        this->treatment = oTreatment.value();
    }

    this->evasionEvaluationString =
        data.value("ban_evasion_evaluation").toString();
    if (const auto oEvaluation = qmagicenum::enumCast<EvasionEvaluation>(
            this->evasionEvaluationString);
        oEvaluation.has_value())
    {
        this->evasionEvaluation = oEvaluation.value();
    }

    for (const auto &rType : data.value("types").toArray())
    {
        if (const auto oRestriction =
                qmagicenum::enumCast<RestrictionType>(rType.toString());
            oRestriction.has_value())
        {
            this->restrictionTypes.set(oRestriction.value());
        }
    }
}

}  // namespace chatterino
