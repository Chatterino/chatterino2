#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/PluginRef.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "debug/AssertInGuiThread.hpp"

namespace chatterino::lua {

struct SignalCallback {
    SignalCallback(PluginWeakRef pluginRef, sol::main_protected_function pfn)
        : pluginRef(std::move(pluginRef))
        , pfn(std::move(pfn))
    {
        assert(this->pfn.valid());
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

    void operator()(auto &&...args) const
    {
        assertInGuiThread();
        auto strong = this->pluginRef.strong();
        if (!strong)
        {
            assert(false && "faulty signal handling");
            return;
        }
        loggedVoidCall(this->pfn, u"SignalCallback::operator()",
                       strong.plugin(), std::forward<decltype(args)>(args)...);
    }

private:
    PluginWeakRef pluginRef;
    sol::main_protected_function pfn;
};

}  // namespace chatterino::lua

#endif
