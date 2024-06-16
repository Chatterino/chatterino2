#include "providers/twitch/pubsubmessages/ChatModeratorAction.hpp"

#include "util/QMagicEnum.hpp"

namespace chatterino {

PubSubChatModeratorActionMessage::PubSubChatModeratorActionMessage(
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
