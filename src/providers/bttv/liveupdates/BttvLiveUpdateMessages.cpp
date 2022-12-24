#include "providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp"

namespace {

bool tryParseChannelId(QString &channelId)
{
    if (!channelId.startsWith("twitch:"))
    {
        return false;
    }

    channelId.remove(0, 7);  // "twitch:"
    return true;
}

}  // namespace

namespace chatterino {

BttvLiveUpdateEmoteUpdateAddMessage::BttvLiveUpdateEmoteUpdateAddMessage(
    const QJsonObject &json)
    : channelID(json["channel"].toString())
    , jsonEmote(json["emote"].toObject())
    , emoteName(this->jsonEmote["code"].toString())
    , emoteID(this->jsonEmote["id"].toString())
    , badChannelID_(!tryParseChannelId(this->channelID))
{
}

bool BttvLiveUpdateEmoteUpdateAddMessage::validate() const
{
    // We don't need to check for jsonEmote["code"]/["id"],
    // because these are this->emoteID and this->emoteName.
    return !this->badChannelID_ && !this->channelID.isEmpty() &&
           !this->emoteID.isEmpty() && !this->emoteName.isEmpty();
}

BttvLiveUpdateEmoteRemoveMessage::BttvLiveUpdateEmoteRemoveMessage(
    const QJsonObject &json)
    : channelID(json["channel"].toString())
    , emoteID(json["emoteId"].toString())
    , badChannelID_(!tryParseChannelId(this->channelID))
{
}

bool BttvLiveUpdateEmoteRemoveMessage::validate() const
{
    return !this->badChannelID_ && !this->emoteID.isEmpty() &&
           !this->channelID.isEmpty();
}

}  // namespace chatterino
