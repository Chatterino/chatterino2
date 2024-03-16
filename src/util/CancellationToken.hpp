#pragma once

#include <atomic>
#include <memory>

namespace chatterino {

/// The CancellationToken is a thread-safe way for worker(s)
/// to know if the task they want to continue doing should be cancelled.
class CancellationToken
{
public:
    CancellationToken() = default;
    explicit CancellationToken(bool isCancelled)
        : isCancelled_(new std::atomic<bool>(isCancelled))
    {
    }

    CancellationToken(const CancellationToken &) = default;
    CancellationToken(CancellationToken &&other)
        : isCancelled_(std::move(other.isCancelled_)){};

    CancellationToken &operator=(CancellationToken &&other)
    {
        this->isCancelled_ = std::move(other.isCancelled_);
        return *this;
    }
    CancellationToken &operator=(const CancellationToken &) = default;

    void cancel()
    {
        if (this->isCancelled_ != nullptr)
        {
            this->isCancelled_->store(true, std::memory_order_release);
        }
    }

    bool isCancelled() const
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
    ScopedCancellationToken(CancellationToken &&backingToken)
        : backingToken_(std::move(backingToken))
    {
    }
    ScopedCancellationToken(CancellationToken backingToken)
        : backingToken_(std::move(backingToken))
    {
    }

    ~ScopedCancellationToken()
    {
        this->backingToken_.cancel();
    }

    ScopedCancellationToken(const ScopedCancellationToken &) = delete;
    ScopedCancellationToken(ScopedCancellationToken &&other)
        : backingToken_(std::move(other.backingToken_)){};
    ScopedCancellationToken &operator=(ScopedCancellationToken &&other)
    {
        this->backingToken_ = std::move(other.backingToken_);
        return *this;
    }
    ScopedCancellationToken &operator=(const ScopedCancellationToken &) =
        delete;

private:
    CancellationToken backingToken_;
};

}  // namespace chatterino
