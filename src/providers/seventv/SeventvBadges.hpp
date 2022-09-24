#pragma once

#include "common/Aliases.hpp"

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include <memory>
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
    std::unordered_map<QString, int> badgeMap_;
    std::vector<EmotePtr> emotes_;
};

}  // namespace chatterino
