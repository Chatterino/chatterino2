#include "providers/twitch/pubsubmessages/ChannelPoints.hpp"

#include "util/QMagicEnum.hpp"

namespace chatterino {

PubSubCommunityPointsChannelV1Message::PubSubCommunityPointsChannelV1Message(
    const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
{
    auto oType = qmagicenum::enumCast<Type>(this->typeString);
    if (oType.has_value())
    {
        this->type = oType.value();
    }
}

}  // namespace chatterino
