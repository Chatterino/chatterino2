#include "providers/twitch/pubsubmessages/ChatroomsUser.hpp"
#include <optional>

#include "common/QLogging.hpp"

namespace chatterino {

PubSubChatroomsUserMessage::PubSubChatroomsUserMessage(const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }

    this->action = data.value("action").toString();
    this->channelID = data.value("channel_id").toString();
    if (this->channelID.isEmpty())
    {
        // Messages for this topic of type channel_banned_alias_restriction_update
        // contain this key in PascalCase instead
        this->channelID = data.value("ChannelID").toString();
    }

    auto eas = data.value("expires_at");
    if (!eas.isUndefined())
    {
        this->expiresAt = QDateTime::fromString(eas.toString(), Qt::ISODate);
    }
    //else
    //{
    //this->expiresAt = boost::none;
    //}

    this->expiresInMs = data.value("expires_in_ms").toInt();
    this->reason = data.value("reason").toString();
    this->targetID = data.value("target_id").toString();

    auto uir = data.value("user_is_restricted");
    if (uir.isBool())
    {
        this->userIsRestricted = uir.toBool();
    }
    //else
    //{
    //this->userIsRestricted = boost::none;
    //}
}
}  // namespace chatterino
