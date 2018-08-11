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
    ConcurrentMap() = default;

    bool tryGet(const TKey &name, TValue &value) const
    {
        QMutexLocker lock(&this->mutex_);

        auto a = this->data_.find(name);
        if (a == this->data_.end()) {
            return false;
        }

        value = a.value();

        return true;
    }

    TValue getOrAdd(const TKey &name, std::function<TValue()> addLambda)
    {
        QMutexLocker lock(&this->mutex_);

        auto a = this->data_.find(name);
        if (a == this->data_.end()) {
            TValue value = addLambda();
            this->data_.insert(name, value);
            return value;
        }

        return a.value();
    }

    TValue &operator[](const TKey &name)
    {
        QMutexLocker lock(&this->mutex_);

        return this->data_[name];
    }

    void clear()
    {
        QMutexLocker lock(&this->mutex_);

        this->data_.clear();
    }

    void insert(const TKey &name, const TValue &value)
    {
        QMutexLocker lock(&this->mutex_);

        this->data_.insert(name, value);
    }

    void each(
        std::function<void(const TKey &name, const TValue &value)> func) const
    {
        QMutexLocker lock(&this->mutex_);

        QMapIterator<TKey, TValue> it(this->data_);

        while (it.hasNext()) {
            it.next();
            func(it.key(), it.value());
        }
    }

    void each(std::function<void(const TKey &name, TValue &value)> func)
    {
        QMutexLocker lock(&this->mutex_);

        QMutableMapIterator<TKey, TValue> it(this->data_);

        while (it.hasNext()) {
            it.next();
            func(it.key(), it.value());
        }
    }

private:
    mutable QMutex mutex_;
    QMap<TKey, TValue> data_;
};

}  // namespace chatterino
