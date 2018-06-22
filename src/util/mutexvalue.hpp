#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>

namespace chatterino {
namespace util {

template <typename T>
class MutexValue : boost::noncopyable
{
    mutable std::mutex mutex;
    T value;

public:
    MutexValue()
    {
    }

    MutexValue(T &&val)
        : value(val)
    {
    }

    T get() const
    {
        std::lock_guard<std::mutex> guard(this->mutex);

        return this->value;
    }

    void set(const T &val)
    {
        std::lock_guard<std::mutex> guard(this->mutex);

        this->value = val;
    }
};

}  // namespace util
}  // namespace chatterino
