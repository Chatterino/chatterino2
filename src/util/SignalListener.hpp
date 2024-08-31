#pragma once

#include "common/ChatterinoSetting.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>

#include <concepts>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace chatterino {

class SignalListener
{
    std::mutex cbMutex;
    std::function<void()> cb;

public:
    using Callback = std::function<void()>;

    explicit SignalListener(Callback callback)
        : cb(std::move(callback))
    {
    }

    ~SignalListener() = default;

    SignalListener(SignalListener &&other) = delete;
    SignalListener &operator=(SignalListener &&other) = delete;
    SignalListener(const SignalListener &) = delete;
    SignalListener &operator=(const SignalListener &) = delete;

    template <typename T>
        requires IsChatterinoSetting<T>
    void add(T &setting)
    {
        setting.connectSimple(
            [this](auto) {
                this->invoke();
            },
            this->managedConnections, false);
    }

    template <typename T>
        requires std::derived_from<T, pajlada::Signals::NoArgSignal>
    void add(T &signal)
    {
        this->managedConnections.emplace_back(
            std::make_unique<pajlada::Signals::ScopedConnection>(
                signal.connect([this] {
                    this->invoke();
                })));
    }

    void invoke()
    {
        std::unique_lock lock(this->cbMutex);

        if (this->cb)
        {
            this->cb();
        }
    }

private:
    std::vector<std::unique_ptr<pajlada::Signals::ScopedConnection>>
        managedConnections;
};

}  // namespace chatterino
