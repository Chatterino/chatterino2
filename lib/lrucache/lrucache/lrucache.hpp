/*
 * File:   lrucache.hpp
 * Original Author: Alexander Ponomarev
 *
 * Created on June 20, 2013, 5:09 PM
 */

#ifndef _LRUCACHE_HPP_INCLUDED_
#define _LRUCACHE_HPP_INCLUDED_

#include <cstddef>
#include <list>
#include <stdexcept>
#include <unordered_map>

namespace cache {

template <typename key_t, typename value_t>
class lru_cache
{
public:
    typedef typename std::pair<key_t, value_t> key_value_pair_t;
    typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

    lru_cache(size_t max_size)
        : _max_size(max_size)
    {
    }

    // copy doesn't make sense since we reference the linked list elements directly
    lru_cache(lru_cache<key_t, value_t> &) = delete;
    lru_cache<key_t, value_t> &operator=(lru_cache<key_t, value_t> &) = delete;

    // move
    lru_cache(lru_cache<key_t, value_t> &&other)
        : _cache_items_list(std::move(other._cache_items_list))
        , _cache_items_map(std::move(other._cache_items_map))
        , _max_size(other._max_size)
    {
        other._cache_items_list.clear();
        other._cache_items_map.clear();
    }

    lru_cache<key_t, value_t> &operator=(lru_cache<key_t, value_t> &&other)
    {
        _cache_items_list = std::move(other._cache_items_list);
        _cache_items_map = std::move(other._cache_items_map);
        _max_size = other._max_size;
        other._cache_items_list.clear();
        other._cache_items_map.clear();
        return *this;
    }

    void put(const key_t &key, const value_t &value)
    {
        auto it = _cache_items_map.find(key);
        _cache_items_list.push_front(key_value_pair_t(key, value));
        if (it != _cache_items_map.end())
        {
            _cache_items_list.erase(it->second);
            _cache_items_map.erase(it);
        }
        _cache_items_map[key] = _cache_items_list.begin();

        if (_cache_items_map.size() > _max_size)
        {
            auto last = _cache_items_list.end();
            last--;
            _cache_items_map.erase(last->first);
            _cache_items_list.pop_back();
        }
    }

    const value_t &get(const key_t &key)
    {
        auto it = _cache_items_map.find(key);
        if (it == _cache_items_map.end())
        {
            throw std::range_error("There is no such key in cache");
        }
        else
        {
            _cache_items_list.splice(_cache_items_list.begin(),
                                     _cache_items_list, it->second);
            return it->second->second;
        }
    }

    bool exists(const key_t &key) const
    {
        return _cache_items_map.find(key) != _cache_items_map.end();
    }

    size_t size() const
    {
        return _cache_items_map.size();
    }

    auto begin() const
    {
        return _cache_items_list.begin();
    }

    auto end() const
    {
        return _cache_items_list.end();
    }

private:
    std::list<key_value_pair_t> _cache_items_list;
    std::unordered_map<key_t, list_iterator_t> _cache_items_map;
    size_t _max_size;
};

}  // namespace cache

#endif /* _LRUCACHE_HPP_INCLUDED_ */
