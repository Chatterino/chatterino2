#pragma once

#include <pajlada/signals/signal.hpp>

#include <mutex>
#include <vector>

namespace chatterino {

template <typename TValue>
class SimpleSignalVector
{
public:
    SimpleSignalVector &operator=(std::vector<TValue> &other)
    {
        this->data_ = other;

        this->updated.invoke();

        return *this;
    }

    operator std::vector<TValue> &()
    {
        return this->data_;
    }

    pajlada::Signals::NoArgSignal updated;

private:
    std::vector<TValue> data_;
};

}  // namespace chatterino
