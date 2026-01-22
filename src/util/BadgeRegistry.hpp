// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/Aliases.hpp"

#include <QJsonObject>

#include <shared_mutex>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class BadgeRegistry
{
public:
    virtual ~BadgeRegistry() = default;

    /// Return the badge, if any, that is assigned to the user
    std::optional<EmotePtr> getBadge(const UserId &id) const;

    /// Assign the given badge to the user
    void assignBadgeToUser(const QString &badgeID, const UserId &userID);

    /// Remove the given badge from the user
    void clearBadgeFromUser(const QString &badgeID, const UserId &userID);

    /// Register a new known badge
    /// The json object will contain all information about the badge, like its ID & its images
    /// @returns The badge's ID
    QString registerBadge(const QJsonObject &badgeJson);

protected:
    BadgeRegistry() = default;

    virtual QString idForBadge(const QJsonObject &badgeJson) const = 0;
    virtual EmotePtr createBadge(const QString &id,
                                 const QJsonObject &badgeJson) const = 0;

private:
    /// Mutex for both `badgeMap_` and `knownBadges_`
    mutable std::shared_mutex mutex_;

    /// user-id => badge
    std::unordered_map<QString, EmotePtr> badgeMap_;
    /// badge-id => badge
    std::unordered_map<QString, EmotePtr> knownBadges_;
};

}  // namespace chatterino
