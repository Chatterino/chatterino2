#pragma once

#include <mutex>

namespace chatterino {

template <typename Type>
class LockedObject
{
public:
    LockedObject &operator=(const LockedObject<Type> &other)
    {
        this->mutex.lock();

        this->data = other.getValue();

        this->mutex.unlock();

        return *this;
    }

    LockedObject &operator=(const Type &other)
    {
        this->mutex.lock();

        this->data = other;

        this->mutex.unlock();

        return *this;
    }

private:
    Type value;
    std::mutex mutex;
};

}  // namespace chatterino
