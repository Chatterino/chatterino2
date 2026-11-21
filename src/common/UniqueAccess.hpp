// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace chatterino {

template <typename T, typename LockType = std::unique_lock<std::shared_mutex>>
class AccessGuard
{
public:
    AccessGuard(T &element, std::shared_mutex &mutex)
        : element_(&element)
        , lock_(mutex)
    {
    }

    AccessGuard(AccessGuard<T, LockType> &&other)
        : element_(other.element_)
        , lock_(std::move(other.lock_))
    {
    }

    AccessGuard<T, LockType> &operator=(AccessGuard<T, LockType> &&other)
    {
        this->element_ = other.element_;
        this->lock_ = std::move(other.lock_);

        return *this;
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
    LockType lock_;
};

template <typename T>
using SharedAccessGuard =
    AccessGuard<const T, std::shared_lock<std::shared_mutex>>;

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
        : element_(std::move(element))
    {
    }

    UniqueAccess<T> &operator=(const T &element)
    {
        this->element_ = element;
        return *this;
    }

    UniqueAccess<T> &operator=(T &&element)
    {
        this->element_ = std::move(element);
        return *this;
    }

    AccessGuard<T> access() const
    {
        return AccessGuard<T>(this->element_, this->mutex_);
    }

    template <typename X = T, typename = std::enable_if_t<!std::is_const_v<X>>>
    SharedAccessGuard<const X> accessConst() const
    {
        return SharedAccessGuard<const T>(this->element_, this->mutex_);
    }

private:
    mutable T element_;
    mutable std::shared_mutex mutex_;
};

}  // namespace chatterino
