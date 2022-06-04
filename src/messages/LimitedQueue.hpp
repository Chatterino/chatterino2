#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <boost/circular_buffer.hpp>

#include <cassert>
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

    // Property Accessors

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

    size_t space() const
    {
        return this->limit() - this->size();
    }

    // Value Accessors
    // copies of values are returned so that references aren't invalidated

    T at(size_t index) const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        assert(index < this->buffer_.size());
        return this->buffer_[index];
    }

    T front() const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        return this->buffer_.front();
    }

    T back() const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        return this->buffer_.back();
    }

    // Modifiers

    void clear()
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        this->buffer_.clear();
    }

    // Pushes an item to the end of the queue. If an element is removed from
    // the front, true is returned and deleted is set.
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

    // Pushes an item to the end of the queue.
    bool pushBack(const T &item)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        bool full = this->buffer_.full();
        this->buffer_.push_back(item);
        return full;
    }

    // Pushes as many items as possible from the end of the given vector, until
    // the queue is full. Returns the subset of items that was pushed.
    std::vector<T> pushFront(const std::vector<T> &items)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        size_t numToPush = std::min(items.size(), this->space());
        std::vector<T> pushed;
        pushed.reserve(numToPush);

        size_t f = items.size() - numToPush;
        size_t b = items.size() - 1;
        for (; f < items.size(); ++f, --b)
        {
            this->buffer_.push_front(items[b]);
            pushed.push_back(items[f]);
        }

        return pushed;
    }

    // Replaces the given item with a replacement. Returns the index of the
    // replacement, or -1 if the item was not found.
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

    // Attempts to replace the item at the given index. Returns whether the
    // replacement succeeded.
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

    // Actions

    // Finds the first item that matches the given predicate. If an item is
    // found, result is set and true is returned.
    template <typename Predicate>
    bool find(T &result, Predicate pred) const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        for (const auto &item : this->buffer_)
        {
            if (pred(item))
            {
                result = item;
                return true;
            }
        }
        return false;
    }

    // Finds the first item that matches the given predicate, starting at the
    // end and working towards the beginning. If an item is found, result is
    // set and true is returned.
    template <typename Predicate>
    bool rfind(T &result, Predicate pred) const
    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        for (auto it = this->buffer_.rbegin(); it != this->buffer_.rend(); ++it)
        {
            if (pred(*it))
            {
                result = *it;
                return true;
            }
        }
        return false;
    }

private:
    mutable std::mutex mutex_;

    size_t limit_;
    boost::circular_buffer<T> buffer_;
};

}  // namespace chatterino
