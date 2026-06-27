#include "controllers/plugins/PluginRef.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/plugins/Plugin.hpp"

#    include <QMessageBox>
#    include <QStringBuilder>

namespace chatterino::lua {

PluginRef::PluginRef(Plugin *plugin)
    : shared(plugin, /*deleter=*/[](void *) {})
{
    assert(plugin != nullptr);
}

PluginRef::PluginRef(std::shared_ptr<Plugin> plugin)
    : shared(std::move(plugin))
{
}

PluginWeakRef PluginRef::weak() const noexcept
{
    return {this->shared};
}

Plugin *PluginRef::plugin() const noexcept
{
    return this->shared.get();
}

void PluginRef::destroy()
{
    auto useCount = this->shared.use_count();
    if (useCount > 1)
    {
        qCWarning(chatterinoLua)
            << "Destroying PluginRef with" << useCount
            << "strong references. Expected one reference (the plugin itself).";
        assert(false && "Too many strong references.");
    }

    this->shared.reset();
}

PluginWeakRef::PluginWeakRef(std::weak_ptr<Plugin> weak)
    : weak(std::move(weak))
{
}

PluginRef PluginWeakRef::strong() const noexcept
{
    return {this->weak.lock()};
}

bool PluginWeakRef::isAlive() const noexcept
{
    return !this->weak.expired();
}

}  // namespace chatterino::lua

#endif
