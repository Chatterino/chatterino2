#pragma once

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include "common/Aliases.hpp"

#include <map>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class FfzBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    FfzBadges();

    boost::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loadFfzBadges();
    std::map<QString, int> badgeMap;
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
