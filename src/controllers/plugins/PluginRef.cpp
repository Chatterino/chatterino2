#include "controllers/plugins/PluginRef.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
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
    if (this->shared.use_count() <= 1)
    {
        this->shared.reset();
        return;
    }

    auto *app = getApp();
    if (app && !app->isTest())
    {
        QMessageBox::critical(
            nullptr, "Chatterino - Plugins",
            "While destroying " % this->plugin()->meta.name %
                ":\nThe shared reference to the plugin was expected to have "
                "one strong reference (the plugin itself). However, it had " %
                QString::number(this->shared.use_count()) %
                ". Thus, it's no longer safe to destroy the "
                "plugin.\nChatterino will exit. You can now attach a debugger "
                "to investigate.");
    }
    std::terminate();
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
