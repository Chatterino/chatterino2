#pragma once

#include "common/Aliases.hpp"
#include "common/Singleton.hpp"
#include "util/QStringHash.hpp"
#include "util/ThreadGuard.hpp"

#include <QColor>

#include <memory>
#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class FfzBadges : public Singleton
{
public:
    void initialize(Settings &settings, const Paths &paths) override;
    FfzBadges() = default;

    struct Badge {
        EmotePtr emote;
        QColor color;
    };

    std::vector<Badge> getUserBadges(const UserId &id);
    std::optional<Badge> getBadge(int badgeID) const;

private:
    void load();

    std::shared_mutex mutex_;

    // userBadges points a user ID to the list of badges they have
    std::unordered_map<QString, std::set<int>> userBadges;

    // badges points a badge ID to the information about the badge
    std::unordered_map<int, Badge> badges;
    ThreadGuard tgBadges;
};

}  // namespace chatterino
