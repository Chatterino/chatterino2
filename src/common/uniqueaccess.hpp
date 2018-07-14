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

    T *operator->() const
    {
        return &this->element_;
    }

    T &operator*() const
    {
        return this->element_;
    }

private:
    T &element_;
    std::mutex &mutex_;
};

template <typename T>
class UniqueAccess
{
public:
    template <typename std::enable_if<std::is_default_constructible<T>::value>::type * = 0>
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

private:
    T element_;
    std::mutex mutex_;
};

}  // namespace chatterino
