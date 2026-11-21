// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"  // IWYU pragma: keep
#include "util/ThreadGuard.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <QColor>
#include <QString>
#include <QVarLengthArray>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class FfzBadges
{
public:
    FfzBadges() = default;

    struct Badge {
        EmotePtr emote;
        QColor color;
    };

    std::vector<Badge> getUserBadges(const UserId &id);
    std::optional<Badge> getBadge(int badgeID) const;

    void registerBadge(int badgeID, Badge badge);
    void assignBadgeToUser(const UserId &userID, int badgeID);

    void load();

private:
    std::shared_mutex mutex_;

    // userBadges points a user ID to the list of badges they have
    boost::unordered_flat_map<QString, QVarLengthArray<int, 2>> userBadges;

    // badges points a badge ID to the information about the badge
    boost::unordered_flat_map<int, Badge> badges;
    ThreadGuard tgBadges;
};

}  // namespace chatterino
