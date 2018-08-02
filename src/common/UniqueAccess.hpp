#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>
#include <type_traits>

namespace chatterino {

template <typename T>
class AccessGuard : boost::noncopyable
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
//    template <typename X = decltype(T())>
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

    template <typename X = T, typename = std::enable_if_t<!std::is_const_v<X>>>
    AccessGuard<const X> accessConst() const
    {
        return AccessGuard<const T>(this->element_, this->mutex_);
    }

private:
    mutable T element_;
    mutable std::mutex mutex_;
};

}  // namespace chatterino
