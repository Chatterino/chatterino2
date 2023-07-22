#pragma once

#include "common/Aliases.hpp"
#include "common/Singleton.hpp"
#include "util/QStringHash.hpp"

#include <boost/optional.hpp>
#include <QJsonObject>

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class SeventvBadges : public Singleton
{
public:
    boost::optional<EmotePtr> getBadge(const UserId &id) const;

    void addBadge(const QJsonObject &badgeJson);
    void assignBadgeToUser(const QString &badgeID, const UserId &userID);
    void clearBadgeFromUser(const QString &badgeID, const UserId &userID);

private:
    // Mutex for both `badgeMap_` and `knownBadges_`
    mutable std::shared_mutex mutex_;

    // user-id => badge
    std::unordered_map<QString, EmotePtr> badgeMap_;
    // badge-id => badge
    std::unordered_map<QString, EmotePtr> knownBadges_;
};

}  // namespace chatterino
