#pragma once

#include <boost/optional.hpp>
#include <unordered_map>
#include "common/Common.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"
#include "messages/EmoteCache.hpp"

namespace chatterino {

class ChatterinoBadges
{
public:
    ChatterinoBadges();

    boost::optional<EmotePtr> getBadge(const UserName &username);

private:
    void loadChatterinoBadges();

    UniqueAccess<EmoteCache<UserName>> badges;
};

}  // namespace chatterino
