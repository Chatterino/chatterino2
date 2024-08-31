#include "common/ChatterSet.hpp"

#include "debug/Benchmark.hpp"

namespace chatterino {

ChatterSet::ChatterSet()
    : items(ChatterSet::CHATTER_LIMIT)
{
}

void ChatterSet::addRecentChatter(const QString &userName)
{
    this->items.put(userName.toLower(), userName);
}

void ChatterSet::updateOnlineChatters(
    const std::unordered_set<QString> &lowerCaseUsernames)
{
    BenchmarkGuard bench("update online chatters");

    // Create a new lru cache without the users that are not present anymore.
    cache::lru_cache<QString, QString> tmp(ChatterSet::CHATTER_LIMIT);

    for (auto &&chatter : lowerCaseUsernames)
    {
        if (this->items.exists(chatter))
        {
            tmp.put(chatter, this->items.get(chatter));

            // Less chatters than the limit => try to preserve as many as possible.
        }
        else if (lowerCaseUsernames.size() < ChatterSet::CHATTER_LIMIT)
        {
            tmp.put(chatter, chatter);
        }
    }

    this->items = std::move(tmp);
}

bool ChatterSet::contains(const QString &userName) const
{
    return this->items.exists(userName.toLower());
}

std::vector<QString> ChatterSet::filterByPrefix(const QString &prefix) const
{
    QString lowerPrefix = prefix.toLower();
    std::vector<QString> result;

    for (auto &&item : this->items)
    {
        if (item.first.startsWith(lowerPrefix))
        {
            result.push_back(item.second);
        }
    }

    return result;
}

std::vector<std::pair<QString, QString>> ChatterSet::all() const
{
    return {this->items.begin(), this->items.end()};
}

}  // namespace chatterino
