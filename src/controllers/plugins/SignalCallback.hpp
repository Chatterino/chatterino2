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

    SignalCallback(const SignalCallback &) = default;
    SignalCallback(SignalCallback &&) = default;
    SignalCallback &operator=(const SignalCallback &) = default;
    SignalCallback &operator=(SignalCallback &&) = default;

    ~SignalCallback()
    {
        assertInGuiThread();
        if (!this->pluginRef.isAlive())
        {
            pfn.abandon();  // don't destruct the function in this case
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
