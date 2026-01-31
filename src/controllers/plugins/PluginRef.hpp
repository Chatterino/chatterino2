#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <memory>

namespace chatterino {
class Plugin;
}  // namespace chatterino

namespace chatterino::lua {

class PluginWeakRef;

/// A strong reference to a plugin.
///
/// This reference is similar to a shared pointer with the big difference that
/// it doesn't own the plugin. Destroying all `PluginRef`s will _not_ destroy
/// the plugin (-> the destructor of the shared_ptr does nothing).
///
/// The plugin itself stores one strong reference. This reference is destroyed
/// with destroy() right before the Lua state is closed. At that point, the
/// reference count must be exactly 1 - meaning only the plugin itself has the
/// strong reference.
///
/// Any object that keeps a reference to the plugin or to Lua values should keep
/// a `PluginWeakRef`. For example, callbacks for signals keep this weak
/// reference and avoid interacting with Lua references if the plugin is dead.
/// While accessing a plugin, objects should upgrade their weak reference to a
/// strong one. This way destroy() can detect potentially dangling references.
class PluginRef
{
public:
    PluginRef() = default;
    PluginRef(PluginRef &&) = default;
    PluginRef &operator=(const PluginRef &) = default;
    PluginRef &operator=(PluginRef &&) = default;
    PluginRef(const PluginRef &) = default;
    ~PluginRef() = default;

    PluginWeakRef weak() const noexcept;
    Plugin *plugin() const noexcept;

    operator bool() const noexcept
    {
        return this->shared != nullptr;
    }

private:
    PluginRef(Plugin *plugin);
    PluginRef(std::shared_ptr<Plugin> plugin);

    /// To be called by the plugin before the Lua state is closed.
    ///
    /// Resets the reference if there are no other shared references.
    /// Otherwise, aborts execution.
    void destroy();

    std::shared_ptr<Plugin> shared;

    friend Plugin;
    friend PluginWeakRef;
};

class PluginWeakRef
{
public:
    PluginWeakRef() = default;
    PluginWeakRef(const PluginWeakRef &) = default;
    PluginWeakRef(PluginWeakRef &&) = default;
    PluginWeakRef &operator=(const PluginWeakRef &) = default;
    PluginWeakRef &operator=(PluginWeakRef &&) = default;
    ~PluginWeakRef() = default;

    PluginRef strong() const noexcept;

    bool isAlive() const noexcept;

private:
    PluginWeakRef(std::weak_ptr<Plugin> weak);

    std::weak_ptr<Plugin> weak;

    friend PluginRef;
};

}  // namespace chatterino::lua

#endif
