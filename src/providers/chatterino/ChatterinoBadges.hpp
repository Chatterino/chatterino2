#pragma once

#include "common/Aliases.hpp"
#include "common/Singleton.hpp"
#include "util/QStringHash.hpp"

#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class ChatterinoBadges : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    ChatterinoBadges();

    std::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loadChatterinoBadges();

    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap;
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
