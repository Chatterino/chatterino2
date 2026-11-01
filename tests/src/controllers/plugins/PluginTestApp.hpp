// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/commands/CommandController.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/WindowManager.hpp"

#include <vector>

namespace chatterino {

class Plugin;
struct PluginPermission;

namespace testlib {

class PluginApplicationPrivate;
class PluginApplication : public mock::BaseApplication
{
public:
    PluginApplication(const QString &settings = "{}");
    ~PluginApplication() override;

    PluginController *getPlugins() final
    {
        return &this->plugins;
    }

    WindowManager *getWindows() final
    {
        return &this->windows;
    }

    CommandController *getCommands() final
    {
        return &this->commands;
    }

    Plugin *loadDefaultPlugin(std::vector<PluginPermission> permissions = {});
    Plugin *loadPlugin(const QString &id,
                       std::vector<PluginPermission> permissions = {});

    bool removePlugin(const QString &id);

private:
    CommandController commands;
    WindowManager windows;
    PluginController plugins;
};

}  // namespace testlib
}  // namespace chatterino
