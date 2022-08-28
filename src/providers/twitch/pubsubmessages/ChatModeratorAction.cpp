#include "providers/twitch/pubsubmessages/ChatModeratorAction.hpp"

namespace chatterino {

PubSubChatModeratorActionMessage::PubSubChatModeratorActionMessage(
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
