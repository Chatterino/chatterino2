#pragma once

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class ChatterinoBadges
{
public:
    /**
     * Makes a network request to load Chatterino user badges
     */
    ChatterinoBadges();

    /**
     * Returns the Chatterino badge for the given user
     */
    std::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loadChatterinoBadges();

    std::shared_mutex mutex_;

    /**
     * Maps Twitch user IDs to their badge index
     * Guarded by mutex_
     */
    std::unordered_map<QString, int> badgeMap;

    /**
     * Keeps a list of badges.
     * Indexes in here are referred to by badgeMap
     */
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
