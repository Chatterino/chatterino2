#pragma once

#include <mutex>

namespace chatterino {

template <typename Type>
class LockedObject
{
public:
    LockedObject &operator=(const LockedObject<Type> &other)
    {
        this->mutex_.lock();

        this->data = other.getValue();

        this->mutex_.unlock();

        return *this;
    }

    LockedObject &operator=(const Type &other)
    {
        this->mutex_.lock();

        this->data = other;

        this->mutex_.unlock();

        return *this;
    }

private:
    Type value_;
    std::mutex mutex_;
};

}  // namespace chatterino
