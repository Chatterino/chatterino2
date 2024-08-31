#pragma once

#include "util/QStringHash.hpp"

#include <lrucache/lrucache.hpp>
#include <QString>

#include <unordered_set>
#include <vector>

namespace chatterino {

/// ChatterSet is a limited container that contains a list of recent chatters
/// that can be referenced by name.
class ChatterSet
{
public:
    /// The limit of how many chatters can be saved for a channel.
    static constexpr size_t CHATTER_LIMIT = 2000;

    ChatterSet();

    /// Inserts a user name if it isn't contained. Doesn't replace the original
    /// if the casing hasn't changed.
    void addRecentChatter(const QString &userName);

    /// Removes chatters that aren't online anymore. Adds chatters that aren't
    /// in the list yet.
    void updateOnlineChatters(
        const std::unordered_set<QString> &lowerCaseUsernames);

    /// Checks if a username is in the list.
    bool contains(const QString &userName) const;

    /// Get filtered usernames by a prefix for autocompletion. Contained items
    /// are in mixed case if available.
    std::vector<QString> filterByPrefix(const QString &prefix) const;

    /// Get all recent chatters. The first pair element contains the username
    /// in lowercase, while the second pair element is the original case.
    std::vector<std::pair<QString, QString>> all() const;

private:
    // user name in lower case -> user name in normal case
    cache::lru_cache<QString, QString> items;
};

using ChatterSet = ChatterSet;

}  // namespace chatterino
