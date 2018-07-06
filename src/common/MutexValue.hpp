#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>

namespace chatterino {

template <typename T>
class MutexValue : boost::noncopyable
{
public:
    MutexValue()
    {
    }

    MutexValue(T &&val)
        : value_(val)
    {
    }

    T get() const
    {
        std::lock_guard<std::mutex> guard(this->mutex_);

        return this->value_;
    }

    void set(const T &val)
    {
        std::lock_guard<std::mutex> guard(this->mutex_);

        this->value_ = val;
    }

private:
    mutable std::mutex mutex_;
    T value_;
};

}  // namespace chatterino
