// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/SignalCallback.hpp"

namespace chatterino::lua {

/// A callback that captures the `self` argument. Used for method calls.
struct SelfCallback {
    SelfCallback() = default;
    SelfCallback(PluginWeakRef pluginRef, sol::main_protected_function pfn,
                 sol::main_object self)
        : cb(std::move(pluginRef), std::move(pfn))
        , self(std::move(self))
    {
        if (!this->self.valid())
        {
            this->self.reset();
            this->cb.reset();
        }
    }

    SelfCallback(const SelfCallback &other) = default;
    SelfCallback(SelfCallback &&) = default;

    SelfCallback &operator=(const SelfCallback &other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (!this->cb.isAlive())
        {
            // Reset the reference before we assign a new reference to it to
            // avoid destroying it.
            this->self.abandon();
        }
        this->cb = other.cb;
        this->self = other.self;
        return *this;
    }

    SelfCallback &operator=(SelfCallback &&other) noexcept
    {
        std::swap(this->cb, other.cb);
        std::swap(this->self, other.self);
        return *this;
    }

    ~SelfCallback()
    {
        if (!this->cb.isAlive())
        {
            this->self.abandon();  // don't destruct the object in this case
        }
    }

    bool isAlive() const
    {
        return this->cb.isAlive();
    }

    template <typename Ret>
    decltype(auto) tryCall(QStringView context, auto &&...args) const
    {
        return this->cb.tryCall<Ret>(context, this->self,
                                     std::forward<decltype(args)>(args)...);
    }

private:
    SignalCallback cb;
    sol::main_object self;
};

}  // namespace chatterino::lua

#endif
