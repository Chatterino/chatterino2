#pragma once

#include "lrucache/lrucache.hpp"
#include "util/QStringHash.hpp"

#include <QString>

#include <unordered_set>

namespace chatterino {

/**
 * @brief ChatterSet is a limited container that contains a list of recent chatters referenceable by name.
 **/
class ChatterSet
{
    /// The limit of how many chatters can be saved for a channel.
    static constexpr size_t CHATTER_LIMIT = 2000;

public:
    /// Inserts a user name if it isn't contained. Doesn't replace the original
    /// if the casing hasn't changed.
    void addRecentChatter(const QString &userName);

    /// Removes chatters that aren't online anymore. Adds chatters that aren't
    /// in the list yet.
    void updateOnlineChatters(
        const std::unordered_set<QString> &lowerCaseUsernames);

    /**
     * @brief Checks if a username is in the list.
     **/
    bool contains(const QString &userName) const;

    /// Get filtered usernames by a prefix for autocompletion. Contained items
    /// are in mixed case if available.
    std::vector<QString> filterByPrefix(const QString &prefix) const;

private:
    // user name in lower case -> user name in normal case
    cache::lru_cache<QString, QString> items_{CHATTER_LIMIT};
};

}  // namespace chatterino
