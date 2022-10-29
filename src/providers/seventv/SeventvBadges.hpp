#pragma once

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class SeventvBadges : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;

    boost::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loadSeventvBadges();

    // Mutex for both `badgeMap_` and `emotes_`
    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap_;
    std::vector<EmotePtr> emotes_;
};

}  // namespace chatterino
