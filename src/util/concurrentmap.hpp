#pragma once

#include <QMap>
#include <QMutex>
#include <QMutexLocker>

#include <functional>
#include <map>
#include <memory>

namespace chatterino {
namespace util {

template <typename TKey, typename TValue>
class ConcurrentMap
{
public:
    ConcurrentMap() = default;

    bool tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(&this->mutex);

        auto a = this->data.find(name);
        if (a == this->data.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(&this->mutex);

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
        QMutexLocker lock(&this->mutex);

        return this->data[name];
    }

    void clear()
    {
        QMutexLocker lock(&this->mutex);

        this->data.clear();
    }

    void insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(&this->mutex);

        this->data.insert(name, value);
    }

    void each(std::function<void(const TKey &name, const TValue &value)> func) const
    {
        QMutexLocker lock(&this->mutex);

        QMapIterator<TKey, TValue> it(this->data);

        while (it.hasNext()) {
            it.next();
            func(it.key(), it.value());
        }
    }

    void each(std::function<void(const TKey &name, TValue &value)> func)
    {
        QMutexLocker lock(&this->mutex);

        QMutableMapIterator<TKey, TValue> it(this->data);

        while (it.hasNext()) {
            it.next();
            func(it.key(), it.value());
        }
    }

private:
    mutable QMutex mutex;
    QMap<TKey, TValue> data;
};

}  // namespace util
}  // namespace chatterino
