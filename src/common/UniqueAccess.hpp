#pragma once

#include <mutex>
#include <type_traits>

namespace chatterino {

template <typename T>
class AccessGuard
{
public:
    AccessGuard(T &element, std::mutex &mutex)
        : element_(element)
        , mutex_(mutex)
    {
        this->mutex_.lock();
    }

    ~AccessGuard()
    {
        this->mutex_.unlock();
    }

    const T *operator->() const
    {
        return &this->element_;
    }

    T *operator->()
    {
        return &this->element_;
    }

    const T &operator*() const
    {
        return this->element_;
    }

    T &operator*()
    {
        return this->element_;
    }

    T clone() const
    {
        return T(this->element_);
    }

private:
    T &element_;
    std::mutex &mutex_;
};

template <typename T>
class UniqueAccess
{
public:
    template <typename X = decltype(T())>
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

    AccessGuard<T> access()
    {
        return AccessGuard<T>(this->element_, this->mutex_);
    }

    const AccessGuard<T> access() const
    {
        return AccessGuard<T>(this->element_, this->mutex_);
    }

private:
    mutable T element_;
    mutable std::mutex mutex_;
};

}  // namespace chatterino
