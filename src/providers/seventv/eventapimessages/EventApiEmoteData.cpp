#include "providers/seventv/eventapimessages/EventApiEmoteData.hpp"

namespace chatterino {
EventApiEmoteData::EventApiEmoteData(QJsonObject _json, QString _baseName)
    : json(std::move(_json))
    , name(this->json.value("name").toString())
    , baseName(std::move(_baseName))
{
}
}  // namespace chatterino
