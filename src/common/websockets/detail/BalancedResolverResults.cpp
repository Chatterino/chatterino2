#include "common/websockets/detail/BalancedResolverResults.hpp"

namespace chatterino::ws::detail {

BalancedResolverResults::BalancedResolverResults(
    const Protocol::resolver::results_type &results)
    : entries(results.begin(), results.end())
{
}

BalancedResolverResults::BalancedResolverResults(std::vector<Entry> entries)
    : entries(std::move(entries))
{
}

std::optional<BalancedResolverResults::Entry>
    BalancedResolverResults::advanceEntry()
{
    if (this->currentIsIPv6)
    {
        // advance v4 first
        if (this->advanceIPv4() || this->advanceIPv6())
        {
            return this->currentEntry();
        }
    }
    else
    {
        if (this->advanceIPv6() || this->advanceIPv4())
        {
            return this->currentEntry();
        }
    }
    return std::nullopt;
}

std::optional<BalancedResolverResults::Entry>
    BalancedResolverResults::currentEntry() const
{
    if (this->currentIdx < this->entries.size())
    {
        return this->entries[this->currentIdx];
    }
    return std::nullopt;
}

void BalancedResolverResults::reset()
{
    this->nextIPv4Idx = 0;
    this->nextIPv6Idx = 0;
    this->currentIdx = std::numeric_limits<size_t>::max();
    this->currentIsIPv6 = true;
}

bool BalancedResolverResults::advanceIPv4()
{
    for (; this->nextIPv4Idx < this->entries.size(); this->nextIPv4Idx++)
    {
        if (this->entries[this->nextIPv4Idx].endpoint().address().is_v4())
        {
            this->currentIdx = this->nextIPv4Idx++;
            this->currentIsIPv6 = false;
            return true;
        }
    }
    return false;
}

bool BalancedResolverResults::advanceIPv6()
{
    for (; this->nextIPv6Idx < this->entries.size(); this->nextIPv6Idx++)
    {
        if (this->entries[this->nextIPv6Idx].endpoint().address().is_v6())
        {
            this->currentIdx = this->nextIPv6Idx++;
            this->currentIsIPv6 = true;
            return true;
        }
    }
    return false;
}

}  // namespace chatterino::ws::detail
