// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/LuaFunctionRef.hpp"

namespace chatterino::lua {

/// A callback that captures the `self` argument. Used for method calls.
struct LuaMethodRef {
    LuaMethodRef() = default;
    LuaMethodRef(PluginWeakRef pluginRef, sol::main_protected_function pfn,
                 sol::main_reference self)
        : fn(std::move(pluginRef), std::move(pfn))
        , self(std::move(self))
    {
        if (!this->self.valid() || !this->isAlive())
        {
            this->reset();
        }
    }

    LuaMethodRef(const LuaMethodRef &other) = default;
    LuaMethodRef(LuaMethodRef &&) = default;

    LuaMethodRef &operator=(const LuaMethodRef &other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (!this->fn.isAlive())
        {
            // Reset the reference before we assign a new reference to it to
            // avoid destroying it.
            this->self.abandon();
        }
        this->fn = other.fn;
        this->self = other.self;
        return *this;
    }

    LuaMethodRef &operator=(LuaMethodRef &&other) noexcept
    {
        std::swap(this->fn, other.fn);
        std::swap(this->self, other.self);
        return *this;
    }

    ~LuaMethodRef()
    {
        if (!this->fn.isAlive())
        {
            // The plugin's Lua state isn't alive anymore - don't attempt to
            // access it in the destructor.
            this->self.abandon();
        }
    }

    bool isAlive() const
    {
        return this->fn.isAlive();
    }

    PluginWeakRef owner() const
    {
        return this->fn.owner();
    }

    void reset()
    {
        if (this->fn.isAlive())
        {
            this->self.reset();
        }
        else
        {
            this->self.abandon();
        }
        this->fn.reset();
    }

    template <typename Ret>
    decltype(auto) tryCall(QStringView context, auto &&...args) const
    {
        return this->fn.tryCall<Ret>(context, this->self,
                                     std::forward<decltype(args)>(args)...);
    }

private:
    LuaFunctionRef fn;
    sol::main_reference self;
};

}  // namespace chatterino::lua

#endif
