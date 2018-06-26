#pragma once

#include <pajlada/signals/signal.hpp>

#include <mutex>
#include <vector>

namespace chatterino {

template <typename TValue>
class SignalVector
{
public:
    SignalVector &operator=(std::vector<TValue> &other)
    {
        this->data = other;

        this->updated.invoke();

        return *this;
    }

    operator std::vector<TValue> &()
    {
        return this->data;
    }

    pajlada::Signals::NoArgSignal updated;

private:
    std::vector<TValue> data;
};

}  // namespace chatterino
