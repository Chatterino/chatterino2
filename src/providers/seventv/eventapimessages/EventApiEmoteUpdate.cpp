#include "providers/seventv/eventapimessages/EventApiEmoteUpdate.hpp"

namespace chatterino {
EventApiEmoteUpdate::EventApiEmoteUpdate(QJsonObject _json)
    : json(std::move(_json))
    , channel(this->json.value("channel").toString())
    , emoteId(this->json.value("emote_id").toString())
    , actor(this->json.value("actor").toString())
    , emoteName(this->json.value("name").toString())
    , actionString(this->json.value("action").toString())
{
    auto action =
        magic_enum::enum_cast<Action>(this->actionString.toStdString());
    if (action.has_value())
    {
        this->action = action.value();
    }

    auto emoteData = this->json.value("emote");
    if (emoteData.isObject())
    {
        auto emoteDataObj = emoteData.toObject();
        // The original name of the emote (not aliased, needed when looking for the emote in the cache).
        auto emoteBaseName = emoteDataObj.value("name").toString();
        // The emote object contains the original name, we need the aliased one.
        emoteDataObj["name"] = this->json["name"];
        // The emote object doesn't contain an id, it's one level above.
        emoteDataObj["id"] = this->json["emote_id"];
        this->emote = EventApiEmoteData(emoteDataObj, emoteBaseName);
    }
    else
    {
        this->emote = boost::none;
    }
}
}  // namespace chatterino
