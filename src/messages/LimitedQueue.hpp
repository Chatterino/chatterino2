#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <cassert>
#include <mutex>
#include <shared_mutex>
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

private:
    /// Property Accessors
    /**
     * @brief Return the limit of the internal buffer
     */
    [[nodiscard]] size_t limit() const
    {
        return this->limit_;
    }

    /**
     * @brief Return the amount of space left in the buffer
     *
     * This does not lock
     */
    [[nodiscard]] size_t space() const
    {
        return this->limit() - this->buffer_.size();
    }

public:
    /**
     * @brief Return true if the buffer is empty
     */
    [[nodiscard]] bool empty() const
    {
        std::shared_lock lock(this->mutex_);

        return this->buffer_.empty();
    }

    /// Value Accessors
    // Copies of values are returned so that references aren't invalidated

    /**
     * @brief Get the item at the given index safely
     *
     * @param[in] index the index of the item to fetch
     * @return the item at the index if it's populated, or none if it's not
     */
    [[nodiscard]] boost::optional<T> get(size_t index) const
    {
        std::shared_lock lock(this->mutex_);

        if (index >= this->buffer_.size())
        {
            return boost::none;
        }

        return this->buffer_[index];
    }

    /**
     * @brief Get the first item from the queue
     *
     * @return the item at the front of the queue if it's populated, or none the queue is empty
     */
    [[nodiscard]] boost::optional<T> first() const
    {
        std::shared_lock lock(this->mutex_);

        if (this->buffer_.empty())
        {
            return boost::none;
        }

        return this->buffer_.front();
    }

    /**
     * @brief Get the last item from the queue
     *
     * @return the item at the back of the queue if it's populated, or none the queue is empty
     */
    [[nodiscard]] boost::optional<T> last() const
    {
        std::shared_lock lock(this->mutex_);

        if (this->buffer_.empty())
        {
            return boost::none;
        }

        return this->buffer_.back();
    }

    /// Modifiers

    // Clear the buffer
    void clear()
    {
        std::unique_lock lock(this->mutex_);

        this->buffer_.clear();
    }

    /**
     * @brief Replaces the contents of the queue with as many items that will fit
     * 
     * If more items than can fit in the queue are provided, the first n that 
     * fit in the queue will be used.
     * 
     * @param items the vector of items to replace with
     */
    void setContents(const std::vector<T> &items)
    {
        std::unique_lock lock(this->mutex_);

        this->buffer_.clear();
        size_t numToPush = std::min(items.size(), this->limit());
        for (size_t i = 0; i < numToPush; ++i)
        {
            this->buffer_.push_back(items[i]);
        }
    }

    /**
     * @brief Replaces the contents of the queue with as many items that will fit
     * 
     * If more items than can fit in the queue are provided, the first n that 
     * fit in the queue will be used.
     * 
     * @param snapshot the snapshot to replace with
     */
    void setContents(const LimitedQueueSnapshot<T> &snapshot)
    {
        std::unique_lock lock(this->mutex_);

        this->buffer_.clear();
        size_t numToPush = std::min(snapshot.size(), this->limit());
        for (size_t i = 0; i < numToPush; ++i)
        {
            this->buffer_.push_back(snapshot[i]);
        }
    }

    /**
     * @brief Push an item to the end of the queue
     *
     * @param item the item to push
     * @param[out] deleted the item that was deleted
     * @return true if an element was deleted to make room
     */
    bool pushBack(const T &item, T &deleted)
    {
        std::unique_lock lock(this->mutex_);

        bool full = this->buffer_.full();
        if (full)
        {
            deleted = this->buffer_.front();
        }
        this->buffer_.push_back(item);
        return full;
    }

    /**
     * @brief Push an item to the end of the queue
     *
     * @param item the item to push
     * @return true if an element was deleted to make room
     */
    bool pushBack(const T &item)
    {
        std::unique_lock lock(this->mutex_);

        bool full = this->buffer_.full();
        this->buffer_.push_back(item);
        return full;
    }

    /**
     * @brief Push items into beginning of queue
     *
     * Items are inserted in reverse order.
     * Items will only be inserted if they fit,
     * meaning no elements can be deleted from using this function.
     *
     * @param items the vector of items to push
     * @return vector of elements that were pushed
     */
    std::vector<T> pushFront(const std::vector<T> &items)
    {
        std::unique_lock lock(this->mutex_);

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

    /**
     * @brief Replace the needle with the given item
     *
     * @param[in] needle the item to search for
     * @param[in] replacement the item to replace needle with
     * @return the index of the replaced item, or -1 if no replacement took place
     */
    template <typename Equals = std::equal_to<T>>
    int replaceItem(const T &needle, const T &replacement)
    {
        std::unique_lock lock(this->mutex_);

        Equals eq;
        for (int i = 0; i < this->buffer_.size(); ++i)
        {
            if (eq(this->buffer_[i], needle))
            {
                this->buffer_[i] = replacement;
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Replace the item at index with the given item
     *
     * @param[in] index the index of the item to replace
     * @param[in] replacement the item to put in place of the item at index
     * @return true if a replacement took place
     */
    bool replaceItem(size_t index, const T &replacement)
    {
        std::unique_lock lock(this->mutex_);

        if (index >= this->buffer_.size())
        {
            return false;
        }

        this->buffer_[index] = replacement;
        return true;
    }

    template <typename Equals = std::equal_to<T>>
    bool insertBefore(const T &needle, const T &item)
    {
        std::unique_lock lock(this->mutex_);

        Equals eq;
        for (auto it = this->buffer_.begin(); it != this->buffer_.end(); ++it)
        {
            if (eq(*it, needle))
            {
                this->buffer_.insert(it, item);
                return true;
            }
        }

        return false;
    }

    template <typename Equals = std::equal_to<T>>
    bool insertAfter(const T &needle, const T &item)
    {
        std::unique_lock lock(this->mutex_);

        Equals eq;
        for (auto it = this->buffer_.begin(); it != this->buffer_.end(); ++it)
        {
            if (eq(*it, needle))
            {
                ++it;  // advance to insert after it
                this->buffer_.insert(it, item);
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] LimitedQueueSnapshot<T> getSnapshot() const
    {
        std::shared_lock lock(this->mutex_);
        return LimitedQueueSnapshot<T>(this->buffer_);
    }

    // Actions

    /**
     * @brief Returns the first item matching a predicate
     * 
     * The contents of the LimitedQueue are iterated over from front to back 
     * until the first element that satisfies `pred(item)`. If no item 
     * satisfies the predicate, or if the queue is empty, then boost::none
     * is returned.
     * 
     * @param[in] pred predicate that will be applied to items
     * @return the first item found or boost::none
     */
    template <typename Predicate>
    [[nodiscard]] boost::optional<T> find(Predicate pred) const
    {
        std::shared_lock lock(this->mutex_);

        for (const auto &item : this->buffer_)
        {
            if (pred(item))
            {
                return item;
            }
        }

        return boost::none;
    }

    /**
     * @brief Returns the first item matching a predicate, checking in reverse
     * 
     * The contents of the LimitedQueue are iterated over from back to front 
     * until the first element that satisfies `pred(item)`. If no item 
     * satisfies the predicate, or if the queue is empty, then boost::none
     * is returned.
     * 
     * @param[in] pred predicate that will be applied to items
     * @return the first item found or boost::none
     */
    template <typename Predicate>
    [[nodiscard]] boost::optional<T> rfind(Predicate pred) const
    {
        std::shared_lock lock(this->mutex_);

        for (auto it = this->buffer_.rbegin(); it != this->buffer_.rend(); ++it)
        {
            if (pred(*it))
            {
                return *it;
            }
        }

        return boost::none;
    }

private:
    mutable std::shared_mutex mutex_;

    const size_t limit_;
    boost::circular_buffer<T> buffer_;
};

}  // namespace chatterino
