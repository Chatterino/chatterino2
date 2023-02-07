#include "PluginController.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/plugins/LuaApi.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"
#    include "singletons/WindowManager.hpp"
#    include "widgets/Notebook.hpp"
#    include "widgets/splits/Split.hpp"
#    include "widgets/Window.hpp"

// lua stuff
#    include "lauxlib.h"
#    include "lua.h"
#    include "lualib.h"

#    include <QJsonDocument>

#    include <memory>
#    include <utility>

namespace chatterino {

void PluginController::initialize(Settings &settings, Paths &paths)
{
    (void)paths;

    // actuallyInitialize will be called by this connection
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
    qCDebug(chatterinoLua) << "Loading plugins in" << dir.path();
    for (const auto &info : dir.entryInfoList(QDir::NoDotAndDotDot))
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
    // look for init.lua
    auto index = QFileInfo(pluginDir.filePath("init.lua"));
    qCDebug(chatterinoLua) << "Looking for init.lua and info.json in"
                           << pluginDir.path();
    if (!index.exists())
    {
        qCDebug(chatterinoLua)
            << "Missing init.lua in plugin directory:" << pluginDir.path();
        return false;
    }
    qCDebug(chatterinoLua) << "Found init.lua, now looking for info.json!";
    auto infojson = QFileInfo(pluginDir.filePath("info.json"));
    if (!infojson.exists())
    {
        qCDebug(chatterinoLua)
            << "Missing info.json in plugin directory" << pluginDir.path();
        return false;
    }
    QFile infoFile(infojson.absoluteFilePath());
    infoFile.open(QIODevice::ReadOnly);
    auto everything = infoFile.readAll();
    auto doc = QJsonDocument::fromJson(everything);
    if (!doc.isObject())
    {
        qCDebug(chatterinoLua)
            << "info.json root is not an object" << pluginDir.path();
        return false;
    }

    auto meta = PluginMeta(doc.object());
    if (!meta.invalidWhy.empty())
    {
        qCDebug(chatterinoLua)
            << "Plugin from" << pluginDir << "is invalid because:";
        for (const auto &why : meta.invalidWhy)
        {
            qCDebug(chatterinoLua) << "- " << why;
        }
        return false;
    }
    this->load(index, pluginDir, meta);
    return true;
}

void PluginController::openLibrariesFor(lua_State *L,
                                        const PluginMeta & /*meta*/)
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

    // NOLINTNEXTLINE
    static const luaL_Reg C2LIB[] = {
        {"system_msg", lua::api::c2_system_msg},
        {"register_command", lua::api::c2_register_command},
        {"send_msg", lua::api::c2_send_msg},
        {nullptr, nullptr},
    };
    lua_pushglobaltable(L);
    auto global = lua_gettop(L);

    // count of elements in C2LIB - 1 (to account for terminator)
    lua::pushEmptyTable(L, 3);

    luaL_setfuncs(L, C2LIB, 0);
    lua_setfield(L, global, "c2");

    // ban functions
    // Note: this might not be fully secure? some kind of metatable fuckery might come up?

    lua_pushglobaltable(L);
    auto gtable = lua_gettop(L);
    lua_getfield(L, gtable, "load");

    // possibly randomize this name at runtime to prevent some attacks?
    lua_setfield(L, LUA_REGISTRYINDEX, "real_load");

    lua_getfield(L, gtable, "dofile");
    lua_setfield(L, LUA_REGISTRYINDEX, "real_dofile");

    // NOLINTNEXTLINE
    static const luaL_Reg replacementFuncs[] = {
        {"load", lua::api::g_load},

        // chatterino dofile is way more similar to require() than dofile()
        {"execfile", lua::api::g_dofile},
        {nullptr, nullptr},
    };
    luaL_setfuncs(L, replacementFuncs, 0);

    lua_pushnil(L);
    lua_setfield(L, gtable, "loadfile");

    lua_pushnil(L);
    lua_setfield(L, gtable, "dofile");

    lua_pop(L, 1);
}

void PluginController::load(const QFileInfo &index, const QDir &pluginDir,
                            const PluginMeta &meta)
{
    lua_State *l = luaL_newstate();
    PluginController::openLibrariesFor(l, meta);

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
    if (!PluginController::isEnabled(pluginName))
    {
        qCInfo(chatterinoLua) << "Skipping loading" << pluginName << "("
                              << meta.name << ") because it is disabled";
        return;
    }
    qCDebug(chatterinoLua) << "Running lua file:" << index;
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
    if (PluginController::isEnabled(codename))
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
    const std::function<void(const std::unique_ptr<Plugin> &pl, lua_State *L)>
        &argCb)
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
