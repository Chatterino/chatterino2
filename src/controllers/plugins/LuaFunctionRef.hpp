// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/PluginRef.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "debug/AssertInGuiThread.hpp"

namespace chatterino::lua {

/// A reference to a Lua function that can outlive a Plugin's lifetime.
struct LuaFunctionRef {
    LuaFunctionRef() = default;
    LuaFunctionRef(PluginWeakRef pluginRef, sol::main_protected_function pfn)
        : pluginRef(std::move(pluginRef))
        , pfn(std::move(pfn))
    {
        if (!this->pfn.valid())
        {
            this->pluginRef = {};
        }
    }

    LuaFunctionRef(const LuaFunctionRef &other) = default;
    LuaFunctionRef(LuaFunctionRef &&) = default;

    LuaFunctionRef &operator=(const LuaFunctionRef &other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (!this->pluginRef.isAlive())
        {
            // Reset the function reference before we assign a new function to
            // it to avoid destroying it.
            this->pfn.abandon();
        }
        this->pluginRef = other.pluginRef;
        this->pfn = other.pfn;
        return *this;
    }

    LuaFunctionRef &operator=(LuaFunctionRef &&other) noexcept
    {
        std::swap(this->pfn, other.pfn);
        std::swap(this->pluginRef, other.pluginRef);
        return *this;
    }

    ~LuaFunctionRef()
    {
        assertInGuiThread();
        if (!this->pluginRef.isAlive())
        {
            this->pfn.abandon();  // don't destruct the function in this case
        }
    }

    PluginWeakRef owner() const
    {
        return this->pluginRef;
    }

    bool isAlive() const
    {
        return this->pluginRef.isAlive();
    }

    void reset()
    {
        if (this->pluginRef.isAlive())
        {
            this->pfn = {};
        }
        else
        {
            this->pfn.abandon();
        }
        this->pluginRef = {};
    }

    /// Try to call this function with `args`.
    ///
    /// @param context Context for logging. Usually the original function name.
    /// @returns `bool` if `Ret` is `void`, `std::optional<Ret>` otherwise.
    template <typename Ret>
    auto tryCall(QStringView context, auto &&...args) const
        -> std::conditional_t<std::is_void_v<Ret>, bool, std::optional<Ret>>
    {
        assertInGuiThread();
        auto strong = this->pluginRef.strong();
        if (!strong)
        {
            return {};  // false / std::nullopt
        }

        auto callResult =
            lua::tryCall<Ret>(this->pfn, std::forward<decltype(args)>(args)...);
        hasValueOrLog(callResult, context, strong.plugin());
        if (callResult)
        {
            if constexpr (std::is_void_v<Ret>)
            {
                return true;
            }
            else
            {
                return std::optional{*std::move(callResult)};
            }
        }

        return {};  // false / std::nullopt
    }

private:
    PluginWeakRef pluginRef;
    sol::main_protected_function pfn;
};

}  // namespace chatterino::lua

#endif
