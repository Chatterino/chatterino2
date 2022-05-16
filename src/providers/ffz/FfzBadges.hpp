#pragma once

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

#include <map>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <QColor>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class FfzBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    FfzBadges() = default;

    boost::optional<EmotePtr> getBadge(const UserId &id);
    boost::optional<QColor> getBadgeColor(const UserId &id);

private:
    void loadFfzBadges();

    std::shared_mutex mutex_;

    std::unordered_map<QString, int> badgeMap;
    std::vector<EmotePtr> badges;
    std::unordered_map<int, QColor> colorMap;
};

}  // namespace chatterino
