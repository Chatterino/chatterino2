#include "providers/seventv/SeventvBadges.hpp"

#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/seventv/SeventvEmotes.hpp"

#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

namespace chatterino {

std::optional<EmotePtr> SeventvBadges::getBadge(const UserId &id) const
{
    std::shared_lock lock(this->mutex_);

    auto it = this->badgeMap_.find(id.string);
    if (it != this->badgeMap_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void SeventvBadges::assignBadgeToUser(const QString &badgeID,
                                      const UserId &userID)
{
    const std::unique_lock lock(this->mutex_);

    const auto badgeIt = this->knownBadges_.find(badgeID);
    if (badgeIt != this->knownBadges_.end())
    {
        this->badgeMap_[userID.string] = badgeIt->second;
    }
}

void SeventvBadges::clearBadgeFromUser(const QString &badgeID,
                                       const UserId &userID)
{
    const std::unique_lock lock(this->mutex_);

    const auto it = this->badgeMap_.find(userID.string);
    if (it != this->badgeMap_.end() && it->second->id.string == badgeID)
    {
        this->badgeMap_.erase(userID.string);
    }
}

void SeventvBadges::registerBadge(const QJsonObject &badgeJson)
{
    const auto badgeID = badgeJson["id"].toString();

    const std::unique_lock lock(this->mutex_);

    if (this->knownBadges_.find(badgeID) != this->knownBadges_.end())
    {
        return;
    }

    auto emote = Emote{
        .name = EmoteName{},
        .images = SeventvEmotes::createImageSet(badgeJson, true),
        .tooltip = Tooltip{badgeJson["tooltip"].toString()},
        .homePage = Url{},
        .id = EmoteId{badgeID},
    };

    if (emote.images.getImage1()->isEmpty())
    {
        return;  // Bad images
    }

    this->knownBadges_[badgeID] =
        std::make_shared<const Emote>(std::move(emote));
}

}  // namespace chatterino
