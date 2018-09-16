#pragma once

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include "common/Aliases.hpp"

#include <map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class ChatterinoBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    ChatterinoBadges();

    boost::optional<EmotePtr> getBadge(const UserName &username);

private:
    void loadChatterinoBadges();
    std::map<QString, int> badgeMap;
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
