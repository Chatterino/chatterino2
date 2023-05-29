#include "providers/twitch/pubsubmessages/LowTrustUsers.hpp"

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
        this->updaterUserID = objUpdatedBy.value("id").toString();
        this->updaterLogin = objUpdatedBy.value("login").toString();
        this->updaterDisplayName = objUpdatedBy.value("display_name").toString();
    }
    else if (this->type == Type::TreatmentUpdate)
    {
        // I kept this empty.
        /*
        this->banEvasionEvaluation = data.value("ban_evasion_evaluation").toString();
        this->treatment = data.value("treatment").toString();
        this->types = data.value("types").toObject();
        
        auto objUpdatedBy = data.value("updated_by").toObject();
        this->updaterUserID = objUpdatedBy.value("id").toString();
        this->updaterLogin = objUpdatedBy.value("login").toString();
        this->updaterDisplayName = objUpdatedBy.value("display_name").toString();

        this->targetUserID = data.value("target_user_id").toString();
        this->targetUsername = data.value("target_user").toString();
        */
    }
}

}  // namespace chatterino