#include "UsernameSet.hpp"

#include <tuple>

namespace chatterino {

namespace {

    std::pair<UsernameSet::Iterator, bool> findOrErase(
        std::set<QString, CaseInsensitiveLess> &set, const QString &value)
    {
        if (!value.isLower())
        {
            auto iter = set.find(value);
            if (iter != set.end())
            {
                if (QString::compare(*iter, value, Qt::CaseSensitive) != 0)
                {
                    set.erase(iter);
                }
                else
                {
                    return {iter, false};
                }
            }
        }
        return {set.end(), true};
    }
}  // namespace

//
// UsernameSet
//

UsernameSet::ConstIterator UsernameSet::begin() const
{
    return this->items.begin();
}

UsernameSet::ConstIterator UsernameSet::end() const
{
    return this->items.end();
}

UsernameSet::Range UsernameSet::subrange(const Prefix &prefix) const
{
    auto it = this->firstKeyForPrefix.find(prefix);
    if (it != this->firstKeyForPrefix.end())
    {
        auto start = this->items.find(it->second);
        auto end = start;

        while (end != this->items.end() && prefix.isStartOf(*end))
        {
            end++;
        }
        return {start, end};
    }

    return {this->items.end(), this->items.end()};
}

std::set<QString>::size_type UsernameSet::size() const
{
    return this->items.size();
}

std::pair<UsernameSet::Iterator, bool> UsernameSet::insert(const QString &value)
{
    auto pair = findOrErase(this->items, value);
    if (!pair.second)
    {
        return pair;
    }

    this->insertPrefix(value);
    return this->items.insert(value);
}

std::pair<UsernameSet::Iterator, bool> UsernameSet::insert(QString &&value)
{
    auto pair = findOrErase(this->items, value);
    if (!pair.second)
    {
        return pair;
    }

    this->insertPrefix(value);
    return this->items.insert(std::move(value));
}

void UsernameSet::insertPrefix(const QString &value)
{
    auto &string = this->firstKeyForPrefix[Prefix(value)];

    if (string.isNull() || value.compare(string, Qt::CaseInsensitive) < 0)
        string = value;
}

bool UsernameSet::contains(const QString &value) const
{
    return this->items.count(value) == 1;
}

void UsernameSet::merge(UsernameSet &&set)
{
    for (auto it = this->items.begin(); it != this->items.end();)
    {
        auto iter = set.items.find(*it);
        if (iter == set.items.end())
        {
            it = this->items.erase(it);
        }
        else
        {
            ++it;
        }
    }
    this->items.merge(set.items);
    this->firstKeyForPrefix = set.firstKeyForPrefix;
}

//
// Range
//

UsernameSet::Range::Range(ConstIterator start, ConstIterator end)
    : start_(start)
    , end_(end)
{
}

UsernameSet::ConstIterator UsernameSet::Range::begin()
{
    return this->start_;
}

UsernameSet::ConstIterator UsernameSet::Range::end()
{
    return this->end_;
}

//
// Prefix
//

Prefix::Prefix(const QString &string)
    : first(string.size() >= 1 ? string[0].toLower() : '\0')
    , second(string.size() >= 2 ? string[1].toLower() : '\0')
{
}

bool Prefix::operator==(const Prefix &other) const
{
    return std::tie(this->first, this->second) ==
           std::tie(other.first, other.second);
}

bool Prefix::operator!=(const Prefix &other) const
{
    return !(*this == other);
}

bool Prefix::isStartOf(const QString &string) const
{
    if (string.size() == 0)
    {
        return this->first == QChar('\0') && this->second == QChar('\0');
    }
    else if (string.size() == 1)
    {
        return this->first == string[0].toLower() &&
               this->second == QChar('\0');
    }
    else
    {
        return this->first == string[0].toLower() &&
               this->second == string[1].toLower();
    }
}

}  // namespace chatterino
