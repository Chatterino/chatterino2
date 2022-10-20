#include "providers/seventv/eventapimessages/SeventvEventApiDispatch.hpp"

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
    , emoteJson(emote)
    , emoteId(this->emoteJson["id"].toString())
{
}

SeventvEventApiEmoteRemoveDispatch::SeventvEventApiEmoteRemoveDispatch(
    const SeventvEventApiDispatch &dispatch, QJsonObject emote)
    : emoteSetId(dispatch.id)
    , actorName(dispatch.actorName)
    , emoteName(emote["name"].toString())
    , emoteId(emote["id"].toString())
{
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
SeventvEventApiUserConnectionUpdateDispatch::
    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update)
    : userId(dispatch.id)
    , actorName(dispatch.actorName)
    , oldEmoteSetId(update["old_value"].toObject()["id"].toString())
    , emoteSetId(update["value"].toObject()["id"].toString())
{
}
}  // namespace chatterino
