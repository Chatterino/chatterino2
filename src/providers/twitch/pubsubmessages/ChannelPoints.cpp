#include "providers/twitch/pubsubmessages/ChannelPoints.hpp"

namespace chatterino {

PubSubCommunityPointsChannelV1Message::PubSubCommunityPointsChannelV1Message(
    const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }
}

}  // namespace chatterino
