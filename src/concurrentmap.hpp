#pragma once

#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include <functional>
#include <map>
#include <memory>

namespace chatterino {

template <typename TKey, typename TValue>
class ConcurrentMap
{
public:
    ConcurrentMap()
    {
    }

    bool tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(this->mutex.get());

        auto a = this->data.find(name);
        if (a == this->data.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(this->mutex.get());

        auto a = this->data.find(name);
        if (a == this->data.end()) {
            TValue value = addLambda();
            this->data.insert(name, value);
            return value;
        }

        return a.value();
    }

    TValue &operator[](const TKey &name)
    {
        QMutexLocker lock(this->mutex.get());

        return this->data[name];
    }

    ConcurrentMap(const ConcurrentMap &o)
        : mutex(std::move(o.mutex))
        , data(std::move(o.data))
    {
    }

    ConcurrentMap &operator=(const ConcurrentMap &rhs)
    {
        this->mutex = std::move(rhs.mutex);
        this->data = std::move(rhs.data);

        return *this;
    }

    void clear()
    {
        QMutexLocker lock(this->mutex.get());

        this->data.clear();
    }

    void insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(this->mutex.get());

        this->data.insert(name, value);
    }

    void each(std::function<void(const TKey &name, const TValue &value)> &func) const
    {
        QMutexLocker lock(this->mutex.get());

        QMapIterator<TKey, TValue> it(this->data);

        while (it.hasNext()) {
            it.next();
            func(it.key(), it.value());
        }
    }

private:
    mutable std::unique_ptr<QMutex> mutex;
    QMap<TKey, TValue> data;
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

    TValue &operator[](const TKey &name)
    {
        QMutexLocker lock(&_mutex);

        return this->_map[name];
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
