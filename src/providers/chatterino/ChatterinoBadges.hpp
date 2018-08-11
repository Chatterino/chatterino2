#pragma once

#include <boost/optional.hpp>

#include "common/Aliases.hpp"

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class ChatterinoBadges
{
public:
    ChatterinoBadges();

    boost::optional<EmotePtr> getBadge(const UserName &username);

private:
    void loadChatterinoBadges();

    // UniqueAccess<EmoteCache<UserName>> badges;
};

}  // namespace chatterino
