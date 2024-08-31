#pragma once

#include <QObject>

#include <tuple>
#include <vector>

template <typename... Args>
class SignalSpy : public QObject
{
    // Simplify the storage if there's only one argument to the signal handler.
    using Call =
        std::conditional_t<sizeof...(Args) == 1,
                           std::tuple_element_t<0, std::tuple<Args...>>,
                           std::tuple<Args...>>;

public:
    SignalSpy(auto *sender, auto method)
    {
        auto conn =
            QObject::connect(sender, method, this, &SignalSpy<Args...>::slot,
                             Qt::DirectConnection);
        assert(conn && "Failed to connect");
    }

    bool empty() const
    {
        return this->calls_.empty();
    }

    size_t size() const
    {
        return this->calls_.size();
    }

    const std::vector<std::tuple<Args...>> &calls() const
    {
        return this->calls_;
    }

    void clear()
    {
        this->calls_.clear();
    }

    const Call &last() const
    {
        assert(!this->calls_.empty());

        return this->calls_.back();
    }

private:
    void slot(Args... args)
    {
        this->calls_.emplace_back(std::move(args)...);
    }

    std::vector<Call> calls_;
};
