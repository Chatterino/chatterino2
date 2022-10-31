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
    : emoteSetID(dispatch.id)
    , actorName(dispatch.actorName)
    , emoteJson(std::move(emote))
    , emoteID(this->emoteJson["id"].toString())
{
}

bool SeventvEventApiEmoteAddDispatch::validate() const
{
    bool validValues =
        !this->emoteSetID.isEmpty() && !this->emoteJson.isEmpty();
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
    : emoteSetID(dispatch.id)
    , actorName(dispatch.actorName)
    , emoteName(emote["name"].toString())
    , emoteID(emote["id"].toString())
{
}

bool SeventvEventApiEmoteRemoveDispatch::validate() const
{
    return !this->emoteSetID.isEmpty() && !this->emoteName.isEmpty() &&
           !this->emoteID.isEmpty();
}

SeventvEventApiEmoteUpdateDispatch::SeventvEventApiEmoteUpdateDispatch(
    const SeventvEventApiDispatch &dispatch, QJsonObject changeField)
    : emoteSetID(dispatch.id)
    , actorName(dispatch.actorName)
{
    auto oldValue = changeField["old_value"].toObject();
    auto value = changeField["value"].toObject();
    this->emoteID = value["id"].toString();
    this->oldEmoteName = oldValue["name"].toString();
    this->emoteName = value["name"].toString();
}

bool SeventvEventApiEmoteUpdateDispatch::validate() const
{
    return !this->emoteSetID.isEmpty() && !this->emoteID.isEmpty() &&
           !this->oldEmoteName.isEmpty() && !this->emoteName.isEmpty() &&
           this->oldEmoteName != this->emoteName;
}

SeventvEventApiUserConnectionUpdateDispatch::
    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update)
    : userID(dispatch.id)
    , actorName(dispatch.actorName)
    , oldEmoteSetID(update["old_value"].toObject()["id"].toString())
    , emoteSetID(update["value"].toObject()["id"].toString())
{
}

bool SeventvEventApiUserConnectionUpdateDispatch::validate() const
{
    return !this->userID.isEmpty() && !this->oldEmoteSetID.isEmpty() &&
           !this->emoteSetID.isEmpty();
}

}  // namespace chatterino
