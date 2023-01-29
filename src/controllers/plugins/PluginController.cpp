#include "PluginController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

#include <memory>
#include <utility>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <qfileinfo.h>
}

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

    luaL_dofile(l, index.absoluteFilePath().toStdString().c_str());

    auto pluginName = pluginDir.dirName();
    auto plugin = std::make_unique<Plugin>(pluginName, l);
    this->plugins.insert({pluginName, std::move(plugin)});
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

constexpr int C_FALSE = 0;
constexpr int C_TRUE = 1;

extern "C" {

int luaC2SystemMsg(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
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
        lua_pushboolean(L, C_FALSE);
        return 1;
    }
    chn->addMessage(makeSystemMessage(text));
    lua_pushboolean(L, C_TRUE);
    return 0;
}

// NOLINTNEXTLINE
static const luaL_Reg C2LIB[] = {
    {"system_msg", luaC2SystemMsg},
    {nullptr, nullptr},
};
}

void PluginController::loadChatterinoLib(lua_State *L)
{
    lua_pushglobaltable(L);
    luaL_setfuncs(L, C2LIB, 0);
}

};  // namespace chatterino
