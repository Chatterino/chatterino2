#include "providers/twitch/pubsubmessages/Base.hpp"

#include "util/QMagicEnum.hpp"

namespace chatterino {

PubSubMessage::PubSubMessage(QJsonObject _object)

    : object(std::move(_object))
    , nonce(this->object.value("nonce").toString())
    , error(this->object.value("error").toString())
    , typeString(this->object.value("type").toString())
{
    auto oType = qmagicenum::enumCast<Type>(this->typeString);
    if (oType.has_value())
    {
        this->type = oType.value();
    }
}

std::optional<PubSubMessage> parsePubSubBaseMessage(const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return std::nullopt;
    }

    return PubSubMessage(jsonDoc.object());
}

}  // namespace chatterino
