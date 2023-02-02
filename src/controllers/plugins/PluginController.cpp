#include "PluginController.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "lauxlib.h"
#    include "lua.h"
#    include "lualib.h"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"
#    include "singletons/WindowManager.hpp"
#    include "widgets/Notebook.hpp"
#    include "widgets/splits/Split.hpp"
#    include "widgets/Window.hpp"

#    include <QJsonDocument>

#    include <memory>
#    include <utility>

namespace chatterino {

void PluginController::initialize(Settings &settings, Paths &paths)
{
    (void)paths;

    settings.enableAnyPlugins.connect([this](bool enabled) {
        if (enabled)
        {
            this->actuallyInitialize();
        }
        else
        {
            // uninitialize plugins
            for (const auto &[codename, plugin] : this->plugins_)
            {
                this->reload(codename);
            }
            // can safely delete them now, after lua freed its stuff
            this->plugins_.clear();
        }
    });
    this->actuallyInitialize();
}

// this function exists to allow for connecting to enableAnyPlugins option
void PluginController::actuallyInitialize()
{
    if (!getSettings()->enableAnyPlugins)
    {
        qCDebug(chatterinoLua)
            << "Loading plugins disabled via Setting, skipping";
        return;
    }
    auto dir = QDir(getPaths()->pluginsDirectory);
    qCDebug(chatterinoLua) << "loading plugins from " << dir;
    for (const auto &info : dir.entryInfoList())
    {
        if (info.isDir())
        {
            auto pluginDir = QDir(info.absoluteFilePath());
            this->tryLoadFromDir(pluginDir);
        }
    }
}
bool PluginController::tryLoadFromDir(const QDir &pluginDir)
{
    // look for index.lua
    auto index = QFileInfo(pluginDir.filePath("index.lua"));
    qCDebug(chatterinoLua) << "looking for index.lua and info.json in"
                           << pluginDir.path();
    if (!index.exists())
    {
        qCDebug(chatterinoLua)
            << "Missing index.lua in plugin directory" << pluginDir;
        return false;
    }
    qCDebug(chatterinoLua) << "found index.lua, now looking for info.json!";
    auto infojson = QFileInfo(pluginDir.filePath("info.json"));
    if (!infojson.exists())
    {
        qCDebug(chatterinoLua)
            << "Missing info.json in plugin directory" << pluginDir;
        return false;
    }
    QFile infoFile(infojson.absoluteFilePath());
    infoFile.open(QIODevice::ReadOnly);
    auto everything = infoFile.readAll();
    auto doc = QJsonDocument::fromJson(everything);
    if (!doc.isObject())
    {
        qCDebug(chatterinoLua)
            << "info.json root is not an object" << pluginDir;
        return false;
    }

    this->load(index, pluginDir, PluginMeta(doc.object()));
    return true;
}
void PluginController::openLibrariesFor(lua_State *L, PluginMeta meta)
{
    // Stuff to change, remove or hide behind a permission system:
    static const std::vector<luaL_Reg> loadedlibs = {
        luaL_Reg{LUA_GNAME, luaopen_base},
        // - print - writes to stdout, should be replaced with a per-plugin log
        // - load, loadstring, loadfile, dofile - don't allow bytecode, *require* valid utf8 (which bytecode by design isn't)

        // luaL_Reg{LUA_LOADLIBNAME, luaopen_package},
        // - explicit fs access, probably best to make our own require() function

        //luaL_Reg{LUA_COLIBNAME, luaopen_coroutine},
        // - needs special support
        luaL_Reg{LUA_TABLIBNAME, luaopen_table},
        // luaL_Reg{LUA_IOLIBNAME, luaopen_io},
        // - explicit fs access, needs wrapper with permissions, no usage ideas yet
        // luaL_Reg{LUA_OSLIBNAME, luaopen_os},
        // - fs access
        // - environ access
        // - exit
        luaL_Reg{LUA_STRLIBNAME, luaopen_string},
        luaL_Reg{LUA_MATHLIBNAME, luaopen_math},
        luaL_Reg{LUA_UTF8LIBNAME, luaopen_utf8},
        // luaL_Reg{LUA_DBLIBNAME, luaopen_debug},
        // - this allows the plugin developer to unleash all hell
    };

    for (const auto &reg : loadedlibs)
    {
        luaL_requiref(L, reg.name, reg.func, int(true));
        lua_pop(L, 1);
    }
}

void PluginController::load(QFileInfo index, QDir pluginDir, PluginMeta meta)
{
    qCDebug(chatterinoLua) << "Running lua file" << index;
    lua_State *l = luaL_newstate();
    this->openLibrariesFor(l, meta);
    this->loadChatterinoLib(l);

    auto pluginName = pluginDir.dirName();
    auto plugin = std::make_unique<Plugin>(pluginName, l, meta, pluginDir);

    for (const auto &[codename, other] : this->plugins_)
    {
        if (other->meta.name == meta.name)
        {
            plugin->isDupeName = true;
            other->isDupeName = true;
        }
    }
    this->plugins_.insert({pluginName, std::move(plugin)});
    if (!this->isEnabled(pluginName))
    {
        qCInfo(chatterinoLua) << "Skipping loading" << pluginName << "("
                              << meta.name << ") because it is disabled";
        return;
    }

    int err = luaL_dofile(l, index.absoluteFilePath().toStdString().c_str());
    if (err != 0)
    {
        qCWarning(chatterinoLua)
            << "Failed to load" << pluginName << "plugin from" << index << ": "
            << lua::humanErrorText(l, err);
        return;
    }
    qCInfo(chatterinoLua) << "Loaded" << pluginName << "plugin from" << index;
}

bool PluginController::reload(const QString &codename)
{
    auto it = this->plugins_.find(codename);
    if (it == this->plugins_.end())
    {
        return false;
    }
    if (it->second->state_ != nullptr)
    {
        lua_close(it->second->state_);
        it->second->state_ = nullptr;
    }
    for (const auto &[cmd, _] : it->second->ownedCommands)
    {
        getApp()->commands->unregisterPluginCommand(cmd);
    }
    it->second->ownedCommands.clear();
    if (this->isEnabled(codename))
    {
        QDir loadDir = it->second->loadDirectory_;
        this->plugins_.erase(codename);
        this->tryLoadFromDir(loadDir);
    }
    return true;
}

void PluginController::callEvery(const QString &functionName)
{
    for (const auto &[name, plugin] : this->plugins_)
    {
        lua_getglobal(plugin->state_, functionName.toStdString().c_str());
        lua_pcall(plugin->state_, 0, 0, 0);
    }
}

void PluginController::callEveryWithArgs(
    const QString &functionName, int count,
    std::function<void(const std::unique_ptr<Plugin> &pl, lua_State *L)> argCb)
{
    for (const auto &[name, plugin] : this->plugins_)
    {
        lua_getglobal(plugin->state_, functionName.toStdString().c_str());
        argCb(plugin, plugin->state_);
        lua_pcall(plugin->state_, count, 0, 0);
    }
}

QString PluginController::tryExecPluginCommand(const QString &commandName,
                                               const CommandContext &ctx)
{
    for (auto &[name, plugin] : this->plugins_)
    {
        if (auto it = plugin->ownedCommands.find(commandName);
            it != plugin->ownedCommands.end())
        {
            const auto &funcName = it->second;

            auto *L = plugin->state_;  // NOLINT
            lua_getfield(L, LUA_REGISTRYINDEX, funcName.toStdString().c_str());
            lua::push(L, ctx);

            auto res = lua_pcall(L, 1, 0, 0);
            if (res != LUA_OK)
            {
                ctx.channel->addMessage(makeSystemMessage(
                    "Lua error: " + lua::humanErrorText(L, res)));
                return "";
            }
            return "";
        }
    }
    qCCritical(chatterinoLua)
        << "Something's seriously up, no plugin owns command" << commandName
        << "yet a call to execute it came in";
    assert(false && "missing plugin command owner");
    return "";
}

extern "C" {

int luaC2SystemMsg(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        qCDebug(chatterinoLua) << "system_msg: need 2 args";
        luaL_error(L, "need exactly 2 arguments");  // NOLINT
        lua::push(L, false);
        return 1;
    }
    QString channel;
    QString text;
    lua::pop(L, &text);
    lua::pop(L, &channel);
    const auto chn = getApp()->twitch->getChannelOrEmpty(channel);
    if (chn->isEmpty())
    {
        qCDebug(chatterinoLua) << "system_msg: no channel" << channel;
        lua::push(L, false);
        return 1;
    }
    qCDebug(chatterinoLua) << "system_msg: OK!";
    chn->addMessage(makeSystemMessage(text));
    lua::push(L, true);
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

    QString name;
    if (!lua::peek(L, &name, 1))
    {
        // NOLINTNEXTLINE
        luaL_error(L, "cannot get string (1st arg of register_command)");
        return 0;
    }
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
int luaC2SendMsg(lua_State *L)
{
    QString text;
    QString channel;
    lua::pop(L, &text);
    lua::pop(L, &channel);

    const auto chn = getApp()->twitch->getChannelOrEmpty(channel);
    if (chn->isEmpty())
    {
        qCDebug(chatterinoLua) << "send_msg: no channel" << channel;
        lua::push(L, false);
        return 1;
    }
    QString message = text;
    message = message.replace('\n', ' ');
    QString outText = getApp()->commands->execCommand(message, chn, false);
    chn->sendMessage(outText);
    lua::push(L, true);
    return 1;
}

// NOLINTNEXTLINE
static const luaL_Reg C2LIB[] = {
    {"system_msg", luaC2SystemMsg},
    {"register_command", luaC2RegisterCommand},
    {"send_msg", luaC2SendMsg},
    {nullptr, nullptr},
};
}

void PluginController::loadChatterinoLib(lua_State *L)
{
    lua_pushglobaltable(L);
    auto global = lua_gettop(L);

    // count of elements in C2LIB - 1 (to account for terminator)
    lua::pushEmptyTable(L, 3);

    luaL_setfuncs(L, C2LIB, 0);
    lua_setfield(L, global, "c2");
}

bool PluginController::isEnabled(const QString &codename)
{
    if (!getSettings()->enableAnyPlugins)
    {
        return false;
    }
    auto vec = getSettings()->enabledPlugins.getValue();
    auto it = std::find(vec.begin(), vec.end(), codename);
    return it != vec.end();
}

};  // namespace chatterino
#endif
