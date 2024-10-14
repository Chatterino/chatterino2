#pragma once

#include "providers/chatterino/ChatterinoBadges.hpp"

#include <unordered_map>

namespace chatterino::mock {

class ChatterinoBadges : public IChatterinoBadges
{
public:
    std::optional<EmotePtr> getBadge(const UserId &id) override
    {
        auto it = this->users.find(id);
        if (it != this->users.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void setBadge(UserId id, EmotePtr emote)
    {
        this->users.emplace(std::move(id), std::move(emote));
    }

private:
    std::unordered_map<UserId, EmotePtr> users;
};

}  // namespace chatterino::mock
