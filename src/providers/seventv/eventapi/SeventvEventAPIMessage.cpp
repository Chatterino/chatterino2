#include "providers/seventv/eventapi/SeventvEventAPIMessage.hpp"

namespace chatterino {

SeventvEventAPIMessage::SeventvEventAPIMessage(QJsonObject _json)
    : data(_json["d"].toObject())
    , op(SeventvEventAPIOpcode(_json["op"].toInt()))
{
}

}  // namespace chatterino
