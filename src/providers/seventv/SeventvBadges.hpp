#pragma once

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

#include <QJsonObject>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class SeventvBadges
{
public:
    // Return the badge, if any, that is assigned to the user
    std::optional<EmotePtr> getBadge(const UserId &id) const;

    // Assign the given badge to the user
    void assignBadgeToUser(const QString &badgeID, const UserId &userID);

    // Remove the given badge from the user
    void clearBadgeFromUser(const QString &badgeID, const UserId &userID);

    // Register a new known badge
    // The json object will contain all information about the badge, like its ID & its images
    void registerBadge(const QJsonObject &badgeJson);

private:
    // Mutex for both `badgeMap_` and `knownBadges_`
    mutable std::shared_mutex mutex_;

    // user-id => badge
    std::unordered_map<QString, EmotePtr> badgeMap_;
    // badge-id => badge
    std::unordered_map<QString, EmotePtr> knownBadges_;
};

}  // namespace chatterino
