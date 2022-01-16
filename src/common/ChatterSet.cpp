#include "common/ChatterSet.hpp"

#include "debug/Benchmark.hpp"

namespace chatterino {

void ChatterSet::addRecentChatter(const QString &userName)
{
    this->items_.put(userName.toLower(), userName);
}

void ChatterSet::updateOnlineChatters(
    const std::unordered_set<QString> &lowerCaseUsernames)
{
    BenchmarkGuard bench("update online chatters");

    // Create a new lru cache without the users that are not present anymore.
    cache::lru_cache<QString, QString> tmp(CHATTER_LIMIT);

    for (auto &&chatter : lowerCaseUsernames)
    {
        if (this->items_.exists(chatter))
        {
            tmp.put(chatter, this->items_.get(chatter));
        }
        else if (lowerCaseUsernames.size() < CHATTER_LIMIT)
        {
            // Less chatters than the limit => try to preserve as many as possible.
            tmp.put(chatter, chatter);
        }
    }

    this->items_ = std::move(tmp);
}

bool ChatterSet::contains(const QString &userName) const
{
    return this->items_.exists(userName.toLower());
}

std::vector<QString> ChatterSet::filterByPrefix(const QString &prefix) const
{
    QString lowerPrefix = prefix.toLower();
    std::vector<QString> result;

    for (auto &&item : this->items_)
    {
        if (item.first.startsWith(lowerPrefix))
        {
            result.push_back(item.second);
        }
    }

    return result;
}

}  // namespace chatterino
