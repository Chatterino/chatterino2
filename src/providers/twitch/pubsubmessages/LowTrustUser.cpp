#include "providers/twitch/pubsubmessages/LowTrustUser.hpp"

namespace chatterino {

PubSubLowTrustUserMessage::PubSubLowTrustUserMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }

    // Check type, because some data doesn't exist depending on type.
    if (this->type == Type::NewMessage)
    {
        auto obj_LowTrustUser = data.value("low_trust_user").toObject();

        this->lowTrustID = obj_LowTrustUser.value("low_trust_id").toString();
        this->channelID = obj_LowTrustUser.value("channel_id").toString();

        auto objSender = obj_LowTrustUser.value("sender").toObject();
        this->senderUserID = objSender.value("user_id").toString();
        this->senderUserLogin = objSender.value("login").toString();
        this->senderDisplayName = objSender.value("display_name").toString();

        this->banEvasionEvaluation = obj_LowTrustUser.value("ban_evasion_evaluation").toString();
        this->treatment = obj_LowTrustUser.value("treatment").toString();
        this->types = obj_LowTrustUser.value("types").toObject();

        this->messageContent = data.value("message_content").toObject();
        this->messageID = data.value("message_id").toString();


        auto objUpdatedBy = obj_LowTrustUser.value("updated_by").toObject();
        this->updater_userID = objUpdatedBy.value("id").toString();
        this->updater_login = objUpdatedBy.value("login").toString();
        this->updater_displayName = objUpdatedBy.value("display_name").toString();
    }
    else if (this->type == Type::TreatmentUpdate)
    {
        // I kept this empty.
        /*
        this->banEvasionEvaluation = data.value("ban_evasion_evaluation").toString();
        this->treatment = data.value("treatment").toString();
        this->types = data.value("types").toObject();
        
        auto objUpdatedBy = data.value("updated_by").toObject();
        this->updater_userID = objUpdatedBy.value("id").toString();
        this->updater_login = objUpdatedBy.value("login").toString();
        this->updater_displayName = objUpdatedBy.value("display_name").toString();

        this->target_userID = data.value("target_user_id").toString();
        this->target_username = data.value("target_user").toString();
        */
    }
}

}  // namespace chatterino