#pragma once

#include <boost/circular_buffer.hpp>

#include <cassert>
#include <memory>
#include <vector>

namespace chatterino {

template <typename T>
class LimitedQueue;

template <typename T>
class LimitedQueueSnapshot
{
private:
    friend class LimitedQueue<T>;

    LimitedQueueSnapshot(const boost::circular_buffer<T> &buf)
        : buffer_(buf.begin(), buf.end())
    {
    }

public:
    LimitedQueueSnapshot() = default;

    size_t size() const
    {
        return this->buffer_.size();
    }

    const T &operator[](size_t index) const
    {
        return this->buffer_[index];
    }

    auto begin() const
    {
        return this->buffer_.begin();
    }

    auto end() const
    {
        return this->buffer_.end();
    }

    auto rbegin() const
    {
        return this->buffer_.rbegin();
    }

    auto rend() const
    {
        return this->buffer_.rend();
    }

private:
    std::vector<T> buffer_;
};

}  // namespace chatterino
