#pragma once

#include <QString>
#include <boost/optional.hpp>
#include <unordered_map>
#include <util/QStringHash.hpp>

#include "common/UniqueAccess.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

template <typename TKey>
class MapReplacement
{
public:
    MapReplacement(std::unordered_map<TKey, EmotePtr> &items)
        : oldItems_(items)
    {
    }

    void add(const TKey &key, const Emote &data)
    {
        this->add(key, Emote(data));
    }

    void add(const TKey &key, Emote &&data)
    {
        auto it = this->oldItems_.find(key);
        if (it != this->oldItems_.end() && *it->second == data) {
            this->newItems_[key] = it->second;
        } else {
            this->newItems_[key] = std::make_shared<Emote>(std::move(data));
        }
    }

    void apply()
    {
        this->oldItems_ = std::move(this->newItems_);
    }

private:
    std::unordered_map<TKey, EmotePtr> &oldItems_;
    std::unordered_map<TKey, EmotePtr> newItems_;
};

template <typename TKey>
class EmoteCache
{
public:
    using Iterator = typename std::unordered_map<TKey, EmotePtr>::iterator;
    using ConstIterator = typename std::unordered_map<TKey, EmotePtr>::iterator;

    Iterator begin()
    {
        return this->items_.begin();
    }

    ConstIterator begin() const
    {
        return this->items_.begin();
    }

    Iterator end()
    {
        return this->items_.end();
    }

    ConstIterator end() const
    {
        return this->items_.end();
    }

    boost::optional<EmotePtr> get(const TKey &key) const
    {
        auto it = this->items_.find(key);

        if (it == this->items_.end())
            return boost::none;
        else
            return it->second;
    }

    MapReplacement<TKey> makeReplacment()
    {
        return MapReplacement<TKey>(this->items_);
    }

private:
    std::unordered_map<TKey, EmotePtr> items_;
};

}  // namespace chatterino
