#include "PluginController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

#include <memory>
#include <utility>

//extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
//}

namespace chatterino {

void PluginController::initialize(Settings &settings, Paths &paths)
{
    (void)(settings);

    auto dir = QDir(paths.pluginsDirectory);
    qCDebug(chatterinoLua) << "loading plugins from " << dir;
    for (const auto &info : dir.entryInfoList())
    {
        if (info.isDir())
        {
            // look for index.lua
            auto pluginDir = QDir(info.absoluteFilePath());
            auto index = QFileInfo(pluginDir.filePath("index.lua"));
            qCDebug(chatterinoLua)
                << "trying" << info << "," << index << "at" << pluginDir;
            if (!index.exists())
            {
                qCDebug(chatterinoLua)
                    << "Missing index.lua in plugin directory" << pluginDir;
                continue;
            }
            qCDebug(chatterinoLua) << "found index.lua, running it!";

            this->load(index, pluginDir);
        }
    }
    //    QApplication::exit();
}

void PluginController::load(QFileInfo index, QDir pluginDir)
{
    qCDebug(chatterinoLua) << "Running lua file" << index;
    lua_State *l = luaL_newstate();
    luaL_openlibs(l);
    this->loadChatterinoLib(l);

    auto pluginName = pluginDir.dirName();
    auto plugin = std::make_unique<Plugin>(pluginName, l);
    this->plugins.insert({pluginName, std::move(plugin)});

    luaL_dofile(l, index.absoluteFilePath().toStdString().c_str());
    qCInfo(chatterinoLua) << "Loaded" << pluginName << "plugin from" << index;
}

void PluginController::callEvery(const QString &functionName)
{
    for (const auto &[name, plugin] : this->plugins)
    {
        lua_getglobal(plugin->state_, functionName.toStdString().c_str());
        lua_pcall(plugin->state_, 0, 0, 0);
    }
}

void PluginController::callEveryWithArgs(
    const QString &functionName, int count,
    std::function<void(const std::unique_ptr<Plugin> &pl, lua_State *L)> argCb)
{
    for (const auto &[name, plugin] : this->plugins)
    {
        lua_getglobal(plugin->state_, functionName.toStdString().c_str());
        argCb(plugin, plugin->state_);
        lua_pcall(plugin->state_, count, 0, 0);
    }
}

QString PluginController::tryExecPluginCommand(const QString &commandName,
                                               const CommandContext &ctx)
{
    for (auto &[name, plugin] : this->plugins)
    {
        if (auto it = plugin->ownedCommands.find(commandName);
            it != plugin->ownedCommands.end())
        {
            const auto &funcName = it->second;

            auto *L = plugin->state_;  // NOLINT
            lua_getfield(L, LUA_REGISTRYINDEX, funcName.toStdString().c_str());
            // put args on stack
            lua_createtable(L, 0, 2);
            auto outIdx = lua_gettop(L);

            lua_createtable(L, ctx.words.count(), 0);
            auto wordsIdx = lua_gettop(L);

            int i = 1;
            for (const auto &w : ctx.words)
            {
                lua_pushstring(L, w.toStdString().c_str());
                lua_seti(L, wordsIdx, i);
                i += 1;
            }

            lua_setfield(L, outIdx, "words");

            lua_pushstring(L, ctx.channel->getName().toStdString().c_str());
            lua_setfield(L, outIdx, "channelName");

            lua_pcall(L, 1, 0, 0);
            return "";
        }
    }
    qCCritical(chatterinoLua)
        << "Something's seriously up, no plugin owns command" << commandName
        << "yet a call to execute it came in";
    assert(false && "missing plugin command owner");
    return "";
}

constexpr int C_FALSE = 0;
constexpr int C_TRUE = 1;

extern "C" {

int luaC2SystemMsg(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        qCDebug(chatterinoLua) << "system_msg: need 2 args";
        luaL_error(L, "need exactly 2 arguments");  // NOLINT
        lua_pushboolean(L, C_FALSE);
        return 1;
    }
    const char *channel = luaL_optstring(L, 1, NULL);
    const char *text = luaL_optstring(L, 2, NULL);
    lua_pop(L, 2);
    const auto chn = getApp()->twitch->getChannelOrEmpty(channel);
    if (chn->isEmpty())
    {
        qCDebug(chatterinoLua) << "system_msg: no channel" << channel;
        lua_pushboolean(L, C_FALSE);
        return 1;
    }
    qCDebug(chatterinoLua) << "system_msg: OK!";
    chn->addMessage(makeSystemMessage(text));
    lua_pushboolean(L, C_TRUE);
    return 1;
}

int luaC2RegisterCommand(lua_State *L)
{
    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "internal error: no plugin");  // NOLINT
        return 0;
    }

    const char *name = luaL_optstring(L, 1, NULL);
    if (lua_isnoneornil(L, 2))
    {
        // NOLINTNEXTLINE
        luaL_error(L, "missing argument for register_command: function "
                      "\"pointer\"");
        return 0;
    }

    auto callbackSavedName = QString("c2commandcb-%1").arg(name);
    lua_setfield(L, LUA_REGISTRYINDEX, callbackSavedName.toStdString().c_str());
    pl->registerCommand(name, callbackSavedName);

    // delete both name and callback
    lua_pop(L, 2);

    return 0;
}

// NOLINTNEXTLINE
static const luaL_Reg C2LIB[] = {
    {"system_msg", luaC2SystemMsg},
    {"register_command", luaC2RegisterCommand},
    {nullptr, nullptr},
};
}

void PluginController::loadChatterinoLib(lua_State *L)
{
    lua_pushglobaltable(L);
    luaL_setfuncs(L, C2LIB, 0);
}

};  // namespace chatterino
