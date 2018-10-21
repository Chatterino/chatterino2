#pragma once

#include <mutex>
#include <type_traits>

namespace chatterino {

template <typename T>
class AccessGuard
{
public:
    AccessGuard(T &element, std::mutex &mutex)
        : element_(&element)
        , mutex_(&mutex)
    {
        this->mutex_->lock();
    }

    AccessGuard(AccessGuard<T> &&other)
        : element_(other.element_)
        , mutex_(other.mutex_)
    {
        other.isValid_ = false;
    }

    AccessGuard<T> &operator=(AccessGuard<T> &&other)
    {
        other.isValid_ = false;
        this->element_ = other.element_;
        this->mutex_ = other.element_;
    }

    ~AccessGuard()
    {
        if (this->isValid_)
            this->mutex_->unlock();
    }

    T *operator->() const
    {
        return this->element_;
    }

    T &operator*() const
    {
        return *this->element_;
    }

private:
    T *element_{};
    std::mutex *mutex_{};
    bool isValid_{true};
};

template <typename T>
class UniqueAccess
{
public:
    UniqueAccess()
        : element_(T())
    {
    }

    UniqueAccess(const T &element)
        : element_(element)
    {
    }

    UniqueAccess(T &&element)
        : element_(element)
    {
    }

    UniqueAccess<T> &operator=(const T &element)
    {
        this->element_ = element;
        return *this;
    }

    UniqueAccess<T> &operator=(T &&element)
    {
        this->element_ = element;
        return *this;
    }

    AccessGuard<T> access() const
    {
        return AccessGuard<T>(this->element_, this->mutex_);
    }

    template <typename X = T,
              typename = std::enable_if_t<!std::is_const<X>::value>>
    AccessGuard<const X> accessConst() const
    {
        return AccessGuard<const T>(this->element_, this->mutex_);
    }

private:
    mutable T element_;
    mutable std::mutex mutex_;
};

}  // namespace chatterino
