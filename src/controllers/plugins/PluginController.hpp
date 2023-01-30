#pragma once

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Singleton.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/CommandController.hpp"
#include "singletons/Paths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QString>

#include <map>
#include <memory>
#include <utility>
#include <vector>

struct lua_State;

namespace chatterino {

class Plugin
{
public:
    QString name;
    Plugin(QString name, lua_State *state)
        : name(std::move(name))
        , state_(state)
    {
    }

    bool registerCommand(const QString &name, const QString &functionName)
    {
        if (this->ownedCommands.find(name) != this->ownedCommands.end())
        {
            return false;
        }

        auto ok = getApp()->commands->registerPluginCommand(name);
        if (!ok)
        {
            return false;
        }
        this->ownedCommands.insert({name, functionName});
        return true;
    }

private:
    lua_State *state_;

    // maps command name -> function name
    std::map<QString, QString> ownedCommands;

    friend class PluginController;
};

class PluginController : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    void save() override{};
    void callEvery(const QString &functionName);
    void callEveryWithArgs(
        const QString &functionName, int count,
        std::function<void(const std::unique_ptr<Plugin> &pl, lua_State *L)>
            argCb);

    QString tryExecPluginCommand(const QString &commandName,
                                 const CommandContext &ctx);

    // NOTE: this pointer does not own the Plugin, unique_ptr still owns it
    // This is required to be public because of c functions
    Plugin *getPluginByStatePtr(lua_State *L)
    {
        for (auto &[name, plugin] : this->plugins)
        {
            if (plugin->state_ == L)
            {
                return plugin.get();
            }
        }
        return nullptr;
    }

private:
    void load(QFileInfo index, QDir pluginDir);
    void loadChatterinoLib(lua_State *l);
    std::map<QString, std::unique_ptr<Plugin>> plugins;
};

};  // namespace chatterino
