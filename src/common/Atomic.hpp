#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>

namespace chatterino
{
    template <typename T>
    class Atomic : boost::noncopyable
    {
    public:
        Atomic()
        {
        }

        Atomic(T&& val)
            : value_(val)
        {
        }

        T get() const
        {
            std::lock_guard<std::mutex> guard(this->mutex_);

            return this->value_;
        }

        void set(const T& val)
        {
            std::lock_guard<std::mutex> guard(this->mutex_);

            this->value_ = val;
        }

        void set(T&& val)
        {
            std::lock_guard<std::mutex> guard(this->mutex_);

            this->value_ = std::move(val);
        }

    private:
        mutable std::mutex mutex_;
        T value_;
    };
}  // namespace chatterino
