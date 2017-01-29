#ifndef CONCURRENTMAP_H
#define CONCURRENTMAP_H

#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include <functional>
#include <unordered_map>

namespace chatterino {

template <typename TKey, typename TValue>
class ConcurrentMap
{
public:
    ConcurrentMap()
        : map()
    {
    }

    bool
    tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(&this->mutex);

        auto a = map.find(name);
        if (a == map.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue
    getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(&this->mutex);

        auto a = map.find(name);
        if (a == map.end()) {
            TValue value = addLambda();
            map.insert(name, value);
            return value;
        }

        return a.value();
    }

    void
    clear()
    {
        QMutexLocker lock(&this->mutex);

        map.clear();
    }

    void
    insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(&this->mutex);

        map.insert(name, value);
    }

private:
    mutable QMutex mutex;
    QMap<TKey, TValue> map;
};

}  // namespace chatterino

#endif  // CONCURRENTMAP_H
