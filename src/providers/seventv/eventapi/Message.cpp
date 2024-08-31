#include "providers/seventv/eventapi/Message.hpp"

namespace chatterino::seventv::eventapi {

Message::Message(QJsonObject _json)
    : data(_json["d"].toObject())
    , op(Opcode(_json["op"].toInt()))
{
}

std::optional<Message> parseBaseMessage(const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return std::nullopt;
    }

    return Message(jsonDoc.object());
}

}  // namespace chatterino::seventv::eventapi
