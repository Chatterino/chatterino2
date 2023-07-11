#pragma once

#include <atomic>
#include <memory>

namespace chatterino {

class CancellationToken
{
public:
    CancellationToken() = default;
    explicit CancellationToken(bool isCanceled)
        : isCanceled_(new std::atomic<bool>(isCanceled))
    {
    }

    CancellationToken(const CancellationToken &) = default;
    CancellationToken(CancellationToken &&other)
        : isCanceled_(std::move(other.isCanceled_)){};

    CancellationToken &operator=(CancellationToken &&other)
    {
        this->isCanceled_ = std::move(other.isCanceled_);
        return *this;
    }
    CancellationToken &operator=(const CancellationToken &) = default;

    void cancel()
    {
        if (this->isCanceled_ != nullptr)
        {
            this->isCanceled_->store(true, std::memory_order_release);
        }
    }

    bool isCanceled() const
    {
        return this->isCanceled_ == nullptr ||
               this->isCanceled_->load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<std::atomic<bool>> isCanceled_;
};

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
