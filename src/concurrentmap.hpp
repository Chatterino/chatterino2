#pragma once

#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include <functional>
#include <map>

namespace chatterino {

template <typename TKey, typename TValue>
class ConcurrentMap
{
public:
    ConcurrentMap()
        : _map()
    {
    }

    bool tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(&_mutex);

        auto a = _map.find(name);
        if (a == _map.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(&_mutex);

        auto a = _map.find(name);
        if (a == _map.end()) {
            TValue value = addLambda();
            _map.insert(name, value);
            return value;
        }

        return a.value();
    }

    void clear()
    {
        QMutexLocker lock(&_mutex);

        _map.clear();
    }

    void insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(&_mutex);

        _map.insert(name, value);
    }

private:
    mutable QMutex _mutex;
    QMap<TKey, TValue> _map;
};

template <typename TKey, typename TValue>
class ConcurrentStdMap
{
public:
    bool tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(&_mutex);

        auto a = _map.find(name);
        if (a == _map.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(&_mutex);

        auto a = _map.find(name);
        if (a == _map.end()) {
            TValue value = addLambda();
            _map.insert(name, value);
            return value;
        }

        return a.value();
    }

    void clear()
    {
        QMutexLocker lock(&_mutex);

        _map.clear();
    }

    void insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(&_mutex);

        _map.insert(name, value);
    }

private:
    mutable QMutex _mutex;
    std::map<TKey, TValue> _map;
};

}  // namespace chatterino
