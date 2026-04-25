#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/PluginRef.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "debug/AssertInGuiThread.hpp"

namespace chatterino::lua {

struct SignalCallback {
    SignalCallback() = default;
    SignalCallback(PluginWeakRef pluginRef, sol::main_protected_function pfn)
        : pluginRef(std::move(pluginRef))
        , pfn(std::move(pfn))
    {
        if (!this->pfn.valid())
        {
            this->reset();
        }
    }

    SignalCallback(const SignalCallback &other) = default;
    SignalCallback(SignalCallback &&) = default;

    SignalCallback &operator=(const SignalCallback &other)
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

    SignalCallback &operator=(SignalCallback &&other) noexcept
    {
        std::swap(this->pfn, other.pfn);
        std::swap(this->pluginRef, other.pluginRef);
        return *this;
    }

    ~SignalCallback()
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

    void operator()(auto &&...args) const
    {
        assert(this->pluginRef.isAlive() && "faulty signal handling");
        this->tryCall<void>(u"SignalCallback::operator()",
                            std::forward<decltype(args)>(args)...);
    }

    template <typename Ret>
    std::optional<Ret> tryCall(QStringView context, auto &&...args) const
    {
        assertInGuiThread();
        auto strong = this->pluginRef.strong();
        std::optional<Ret> ret;
        if (!strong)
        {
            return ret;
        }

        auto callResult =
            lua::tryCall<Ret>(this->pfn, std::forward<decltype(args)>(args)...);
        hasValueOrLog(callResult, context, strong.plugin());
        if (callResult)
        {
            ret.emplace(*std::move(callResult));
        }
        return ret;
    }

    template <typename Ret>
    bool tryCall(QStringView context, auto &&...args) const
        requires std::is_void_v<Ret>
    {
        assertInGuiThread();
        auto strong = this->pluginRef.strong();
        if (!strong)
        {
            return false;
        }

        auto callResult =
            lua::tryCall<Ret>(this->pfn, std::forward<decltype(args)>(args)...);
        hasValueOrLog(callResult, context, strong.plugin());

        return callResult.has_value();
    }

private:
    PluginWeakRef pluginRef;
    sol::main_protected_function pfn;
};

}  // namespace chatterino::lua

#endif
