#ifndef CONCURRENTMAP_H
#define CONCURRENTMAP_H

#include <QMutex>
#include <QMap>
#include <functional>

template<typename TKey, typename TValue>
class ConcurrentMap
{
public:
    ConcurrentMap() {
        mutex = new QMutex();
        map = new QMap<TKey, TValue>();
    }

    bool tryGet(const TKey &name, TValue& value) {
        mutex->lock();
        auto a = map->find(name);
        if (a == map->end()) {
            mutex->unlock();
            value = NULL;
            return false;
        }
        mutex->unlock();
        value = a.value();
        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue ()> addLambda) {
        mutex->lock();
        auto a = map->find(name);
        if (a == map->end()) {
            TValue value = addLambda();
            map->insert(name, value);
            mutex->unlock();
            return value;
        }
        mutex->unlock();
        return a.value();
    }

    void clear() {
        mutex->lock();
        map->clear();
        mutex->unlock();
    }

    void insert(const TKey &name, const TValue &value) {
        mutex->lock();
        map->insert(name, value);
        mutex->unlock();
    }

private:
    QMutex* mutex;
    QMap<TKey, TValue>* map;
};

#endif // CONCURRENTMAP_H
