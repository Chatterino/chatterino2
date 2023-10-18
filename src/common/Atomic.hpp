#pragma once

#include <mutex>

namespace chatterino {

template <typename T>
class Atomic
{
public:
    Atomic() = default;

    Atomic(T &&val)
        : value_(val)
    {
    }

    Atomic(const Atomic &) = delete;
    Atomic &operator=(const Atomic &) = delete;

    Atomic(Atomic &&) = delete;
    Atomic &operator=(Atomic &&) = delete;

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

    void set(T &&val)
    {
        std::lock_guard<std::mutex> guard(this->mutex_);

        this->value_ = std::move(val);
    }

private:
    mutable std::mutex mutex_;
    T value_;
};

}  // namespace chatterino
