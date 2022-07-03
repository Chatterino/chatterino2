#include "providers/seventv/eventapimessages/EventApiMessage.hpp"

namespace chatterino {
EventApiMessage::EventApiMessage(QJsonObject _json)
    : json(std::move(_json))
    , actionString(this->json.value("action").toString())
{
    auto action =
        magic_enum::enum_cast<Action>(this->actionString.toStdString());
    if (action.has_value())
    {
        this->action = action.value();
    }
}
}  // namespace chatterino
