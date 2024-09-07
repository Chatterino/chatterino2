#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace chatterino {

/// The CancellationToken is a thread-safe way for worker(s)
/// to know if the task they want to continue doing should be cancelled.
class CancellationToken
{
public:
    CancellationToken() noexcept = default;
    explicit CancellationToken(bool isCancelled)
        : isCancelled_(new std::atomic<bool>(isCancelled))
    {
    }

    CancellationToken(const CancellationToken &) = default;
    CancellationToken(CancellationToken &&other) noexcept
        : isCancelled_(std::move(other.isCancelled_)){};

    /// @brief This destructor doesn't cancel the token
    ///
    /// @see ScopedCancellationToken
    /// @see #cancel()
    ~CancellationToken() noexcept = default;

    CancellationToken &operator=(CancellationToken &&other) noexcept
    {
        this->isCancelled_ = std::move(other.isCancelled_);
        return *this;
    }
    CancellationToken &operator=(const CancellationToken &) = default;

    void cancel() noexcept
    {
        if (this->isCancelled_ != nullptr)
        {
            this->isCancelled_->store(true, std::memory_order_release);
        }
    }

    bool isCancelled() const noexcept
    {
        return this->isCancelled_ == nullptr ||
               this->isCancelled_->load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<std::atomic<bool>> isCancelled_;
};

/// The ScopedCancellationToken is a way to automatically cancel a CancellationToken when it goes out of scope
class ScopedCancellationToken
{
public:
    ScopedCancellationToken() = default;
    explicit ScopedCancellationToken(CancellationToken backingToken)
        : backingToken_(std::move(backingToken))
    {
    }

    ScopedCancellationToken(const ScopedCancellationToken &) = delete;
    ScopedCancellationToken(ScopedCancellationToken &&other) noexcept
        : backingToken_(std::move(other.backingToken_)){};

    ~ScopedCancellationToken()
    {
        this->backingToken_.cancel();
    }

    ScopedCancellationToken &operator=(CancellationToken token) noexcept
    {
        this->backingToken_.cancel();
        this->backingToken_ = std::move(token);
        return *this;
    }

    ScopedCancellationToken &operator=(const ScopedCancellationToken &) =
        delete;
    ScopedCancellationToken &operator=(ScopedCancellationToken &&other) noexcept
    {
        this->backingToken_.cancel();
        this->backingToken_ = std::move(other.backingToken_);
        return *this;
    }

private:
    CancellationToken backingToken_;
};

}  // namespace chatterino
