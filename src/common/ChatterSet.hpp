#pragma once

#include <QString>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "lrucache/lrucache.hpp"
#include "util/QStringHash.hpp"

namespace chatterino {

/// ChatterSet is a limited container that contains a list of recent chatters
/// that can be referenced by name.
class ChatterSet
{
public:
    /// The limit of how many chatters can be saved for a channel.
    static constexpr size_t chatterLimit = 2000;

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

private:
    // user name in lower case -> user name in normal case
    cache::lru_cache<QString, QString> items;
};

using ChatterSet = ChatterSet;

}  // namespace chatterino
