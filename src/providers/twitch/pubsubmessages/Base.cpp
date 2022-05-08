#include "providers/twitch/pubsubmessages/Base.hpp"

namespace chatterino {

PubSubMessage::PubSubMessage(QJsonObject _object)

    : object(std::move(_object))
    , nonce(this->object.value("nonce").toString())
    , error(this->object.value("error").toString())
    , typeString(this->object.value("type").toString())
{
    auto oType = magic_enum::enum_cast<Type>(this->typeString.toStdString());
    if (oType.has_value())
    {
        this->type = oType.value();
    }
}

}  // namespace chatterino
