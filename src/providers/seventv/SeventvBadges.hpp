#pragma once

#include "common/Aliases.hpp"

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include <map>
#include <memory>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class SeventvBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    SeventvBadges();

    boost::optional<EmotePtr> getBadge(const UserId &id);

private:
    void loadSeventvBadges();
    std::map<QString, int> badgeMap;
    std::vector<EmotePtr> emotes;
};

}  // namespace chatterino
