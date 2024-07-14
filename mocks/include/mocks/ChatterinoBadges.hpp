#pragma once

#include "providers/chatterino/ChatterinoBadges.hpp"

namespace chatterino::mock {

class ChatterinoBadges : public IChatterinoBadges
{
public:
    std::optional<EmotePtr> getBadge(const UserId &id) override
    {
        (void)id;
        return std::nullopt;
    }
};

}  // namespace chatterino::mock
