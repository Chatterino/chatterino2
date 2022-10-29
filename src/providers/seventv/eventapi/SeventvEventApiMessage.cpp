#include "providers/seventv/eventapi/SeventvEventApiMessage.hpp"

namespace chatterino {

SeventvEventApiMessage::SeventvEventApiMessage(QJsonObject _json)
    : data(_json["d"].toObject())
    , op(SeventvEventApiOpcode(_json["op"].toInt()))
{
}

}  // namespace chatterino
