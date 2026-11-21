// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/PluginTestApp.hpp"

using namespace Qt::Literals;

namespace chatterino {

class PluginControllerAccess
{
public:
    static void openLibrariesFor(Plugin *plugin)
    {
        getApp()->getPlugins()->openLibrariesFor(plugin);
    }

    static std::map<QString, AnyPlugin> &plugins()
    {
        return getApp()->getPlugins()->plugins_;
    }
};

namespace testlib {

PluginApplication::PluginApplication(const QString &settings)
    : mock::BaseApplication(settings)
    , commands(this->paths_)
    , windows(this->args_, this->paths_, this->settings, this->theme,
              this->fonts)
    , plugins(this->paths_)
{
}

PluginApplication::~PluginApplication() = default;

Plugin *PluginApplication::loadDefaultPlugin(
    std::vector<PluginPermission> permissions)
{
    return this->loadPlugin(u"test"_s, std::move(permissions));
}

Plugin *PluginApplication::loadPlugin(const QString &id,
                                      std::vector<PluginPermission> permissions)
{
    auto &plugins = PluginControllerAccess::plugins();
    PluginMeta meta;
    meta.name = id;
    meta.license = u"MIT"_s;
    meta.homepage = u"https://github.com/Chatterino/chatterino2"_s;
    meta.description = u"Plugin for tests"_s;
    meta.permissions = std::move(permissions);

    QDir plugindir = QDir(this->paths_.pluginsDirectory).absoluteFilePath(id);

    plugindir.mkpath(u"."_s);
    auto temp = std::make_unique<Plugin>(id, luaL_newstate(), meta, plugindir);
    auto *plug = temp.get();
    plugins.insert({id, std::move(temp)});

    // This skips PluginController::load().
    PluginControllerAccess::openLibrariesFor(plug);
    return plug;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool PluginApplication::removePlugin(const QString &id)
{
    auto &plugins = PluginControllerAccess::plugins();
    return plugins.erase(id) > 0;
}

}  // namespace testlib
}  // namespace chatterino
