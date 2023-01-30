#include "providers/seventv/eventapi/Message.hpp"

namespace chatterino::seventv::eventapi {

Message::Message(QJsonObject _json)
    : data(_json["d"].toObject())
    , op(Opcode(_json["op"].toInt()))
{
}

}  // namespace chatterino::seventv::eventapi
