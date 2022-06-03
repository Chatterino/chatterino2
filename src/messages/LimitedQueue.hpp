#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <boost/circular_buffer.hpp>
#include <mutex>
#include <vector>

namespace chatterino {

template <typename T>
class LimitedQueue
{
public:
    LimitedQueue(size_t limit = 1000)
        : limit_(limit)
        , buffer_(limit)
    {
    }

    size_t size() const
    {
        return this->buffer_.size();
    }

    size_t limit() const
    {
        return this->limit_;
    }

    bool empty() const
    {
        return this->buffer_.empty();
    }

    bool full() const
    {
        return this->buffer_.full();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        this->buffer_.clear();
    }

    bool pushBack(const T &item, T &deleted)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        bool full = this->buffer_.full();
        if (full)
        {
            deleted = this->buffer_.front();
        }
        this->buffer_.push_back(item);
        return full;
    }

    bool pushBack(const T &item)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        bool full = this->buffer_.full();
        if (full)
        {
            this->buffer_.front();
        }
        this->buffer_.push_back(item);
        return full;
    }

    std::vector<T> pushFront(const std::vector<T> &items)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        std::vector<T> pushed;
        pushed.reserve(
            std::min(items.size(), this->limit_ - this->buffer_.size()));

        for (auto it = items.crbegin(); it != items.crend(); ++it)
        {
            if (this->buffer_.full())
            {
                break;
            }

            this->buffer_.push_front(*it);
            pushed.push_back(*it);
        }

        return pushed;
    }

    int replaceItem(const T &item, const T &replacement)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        for (int i = 0; i < this->buffer_.size(); ++i)
        {
            if (this->buffer_[i] == item)
            {
                this->buffer_[i] = replacement;
                return i;
            }
        }
        return -1;
    }

    bool replaceItem(size_t index, const T &replacement)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        if (index >= this->buffer_.size())
        {
            return false;
        }

        this->buffer_[index] = replacement;
        return true;
    }

    LimitedQueueSnapshot<T> getSnapshot() const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        return LimitedQueueSnapshot<T>(this->buffer_);
    }

private:
    mutable std::mutex mutex_;

    size_t limit_;
    boost::circular_buffer<T> buffer_;
};

}  // namespace chatterino
