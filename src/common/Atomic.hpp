#pragma once

#include <atomic>
#include <memory>
#include <mutex>

namespace chatterino {

template <typename T>
class Atomic
{
public:
    Atomic() = default;
    ~Atomic() = default;

    Atomic(T &&val)
        : value_(std::move(val))
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

#if defined(__cpp_lib_atomic_shared_ptr) && defined(__cpp_concepts)

template <typename T>
class Atomic<std::shared_ptr<T>>
{
    // Atomic<std::shared_ptr<T>> must be instantated with a const T
};

template <typename T>
    requires std::is_const_v<T>
class Atomic<std::shared_ptr<T>>
{
public:
    Atomic() = default;
    ~Atomic() = default;

    Atomic(T &&val)
        : value_(std::make_shared<T>(std::move(val)))
    {
    }

    Atomic(std::shared_ptr<T> &&val)
        : value_(std::move(val))
    {
    }

    Atomic(const Atomic &) = delete;
    Atomic &operator=(const Atomic &) = delete;

    Atomic(Atomic &&) = delete;
    Atomic &operator=(Atomic &&) = delete;

    std::shared_ptr<T> get() const
    {
        return this->value_.load();
    }

    void set(const T &val)
    {
        this->value_.store(std::make_shared<T>(val));
    }

    void set(T &&val)
    {
        this->value_.store(std::make_shared<T>(std::move(val)));
    }

    void set(const std::shared_ptr<T> &val)
    {
        this->value_.store(val);
    }

    void set(std::shared_ptr<T> &&val)
    {
        this->value_.store(std::move(val));
    }

private:
    std::atomic<std::shared_ptr<T>> value_;
};

#endif

}  // namespace chatterino
