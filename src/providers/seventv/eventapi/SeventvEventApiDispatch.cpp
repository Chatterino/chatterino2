#include <utility>

#include "providers/seventv/eventapi/SeventvEventApiDispatch.hpp"

namespace chatterino {

SeventvEventApiDispatch::SeventvEventApiDispatch(QJsonObject obj)
    : body(obj["body"].toObject())
    , id(this->body["id"].toString())
    , actorName(this->body["actor"].toObject()["display_name"].toString())
{
    auto subType = magic_enum::enum_cast<SeventvEventApiSubscriptionType>(
        obj["type"].toString().toStdString());
    if (subType.has_value())
    {
        this->type = subType.value();
    }
}

SeventvEventApiEmoteAddDispatch::SeventvEventApiEmoteAddDispatch(
    const SeventvEventApiDispatch &dispatch, QJsonObject emote)
    : emoteSetId(dispatch.id)
    , actorName(dispatch.actorName)
    , emoteJson(std::move(emote))
    , emoteId(this->emoteJson["id"].toString())
{
}

bool SeventvEventApiEmoteAddDispatch::validate()
{
    bool validValues =
        !this->emoteSetId.isEmpty() && !this->emoteJson.isEmpty();
    if (!validValues)
    {
        return false;
    }
    bool validActiveEmote = this->emoteJson.contains("id") &&
                            this->emoteJson.contains("name") &&
                            this->emoteJson.contains("data");
    if (!validActiveEmote)
    {
        return false;
    }
    auto emoteData = this->emoteJson["data"].toObject();
    return emoteData.contains("name") && emoteData.contains("host") &&
           emoteData.contains("owner");
}

SeventvEventApiEmoteRemoveDispatch::SeventvEventApiEmoteRemoveDispatch(
    const SeventvEventApiDispatch &dispatch, QJsonObject emote)
    : emoteSetId(dispatch.id)
    , actorName(dispatch.actorName)
    , emoteName(emote["name"].toString())
    , emoteId(emote["id"].toString())
{
}

bool SeventvEventApiEmoteRemoveDispatch::validate()
{
    return !this->emoteSetId.isEmpty() && !this->emoteName.isEmpty() &&
           !this->emoteId.isEmpty();
}

SeventvEventApiEmoteUpdateDispatch::SeventvEventApiEmoteUpdateDispatch(
    const SeventvEventApiDispatch &dispatch, QJsonObject changeField)
    : emoteSetId(dispatch.id)
    , actorName(dispatch.actorName)
{
    auto oldValue = changeField["old_value"].toObject();
    auto value = changeField["value"].toObject();
    this->emoteId = value["id"].toString();
    this->oldEmoteName = oldValue["name"].toString();
    this->emoteName = value["name"].toString();
}

bool SeventvEventApiEmoteUpdateDispatch::validate()
{
    return !this->emoteSetId.isEmpty() && !this->emoteId.isEmpty() &&
           !this->oldEmoteName.isEmpty() && !this->emoteName.isEmpty() &&
           this->oldEmoteName != this->emoteName;
}

SeventvEventApiUserConnectionUpdateDispatch::
    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update)
    : userId(dispatch.id)
    , actorName(dispatch.actorName)
    , oldEmoteSetId(update["old_value"].toObject()["id"].toString())
    , emoteSetId(update["value"].toObject()["id"].toString())
{
}

bool SeventvEventApiUserConnectionUpdateDispatch::validate()
{
    return !this->userId.isEmpty() && !this->oldEmoteSetId.isEmpty() &&
           !this->emoteSetId.isEmpty();
}

}  // namespace chatterino
