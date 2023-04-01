#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginController.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <lualib.h>
#    include <QJsonDocument>

#    include <memory>
#    include <utility>

namespace chatterino {

void PluginController::initialize(Settings &settings, Paths &paths)
{
    (void)paths;

    // actuallyInitialize will be called by this connection
    settings.pluginsEnabled.connect([this](bool enabled) {
        if (enabled)
        {
            this->loadPlugins();
        }
        else
        {
            // uninitialize plugins
            this->plugins_.clear();
        }
    });
}

void PluginController::loadPlugins()
{
    this->plugins_.clear();
    auto dir = QDir(getPaths()->pluginsDirectory);
    qCDebug(chatterinoLua) << "Loading plugins in" << dir.path();
    for (const auto &info :
         dir.entryInfoList(QDir::NoFilter | QDir::NoDotAndDotDot))
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
        qCWarning(chatterinoLua)
            << "Missing init.lua in plugin directory:" << pluginDir.path();
        return false;
    }
    qCDebug(chatterinoLua) << "Found init.lua, now looking for info.json!";
    auto infojson = QFileInfo(pluginDir.filePath("info.json"));
    if (!infojson.exists())
    {
        qCWarning(chatterinoLua)
            << "Missing info.json in plugin directory" << pluginDir.path();
        return false;
    }
    QFile infoFile(infojson.absoluteFilePath());
    infoFile.open(QIODevice::ReadOnly);
    auto everything = infoFile.readAll();
    auto doc = QJsonDocument::fromJson(everything);
    if (!doc.isObject())
    {
        qCWarning(chatterinoLua)
            << "info.json root is not an object" << pluginDir.path();
        return false;
    }

    auto meta = PluginMeta(doc.object());
    if (!meta.isValid())
    {
        qCWarning(chatterinoLua)
            << "Plugin from" << pluginDir << "is invalid because:";
        for (const auto &why : meta.errors)
        {
            qCWarning(chatterinoLua) << "- " << why;
        }
        auto plugin = std::make_unique<Plugin>(pluginDir.dirName(), nullptr,
                                               meta, pluginDir);
        this->plugins_.insert({pluginDir.dirName(), std::move(plugin)});
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
        // - load - don't allow in release mode

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
    };
    // Warning: Do not add debug library to this, it would make the security of
    // this a living nightmare due to stuff like registry access
    // - Mm2PL

    for (const auto &reg : loadedlibs)
    {
        luaL_requiref(L, reg.name, reg.func, int(true));
        lua_pop(L, 1);
    }

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    static const luaL_Reg c2Lib[] = {
        {"system_msg", lua::api::c2_system_msg},
        {"register_command", lua::api::c2_register_command},
        {"send_msg", lua::api::c2_send_msg},
        {"log", lua::api::c2_log},
        {nullptr, nullptr},
    };
    lua_pushglobaltable(L);
    auto global = lua_gettop(L);

    // count of elements in C2LIB + LogLevel
    auto c2libIdx = lua::pushEmptyTable(L, 5);

    luaL_setfuncs(L, c2Lib, 0);

    lua::pushEnumTable<lua::api::LogLevel>(L);
    lua_setfield(L, c2libIdx, "LogLevel");

    lua_setfield(L, global, "c2");

    // ban functions
    // Note: this might not be fully secure? some kind of metatable fuckery might come up?

    lua_pushglobaltable(L);
    auto gtable = lua_gettop(L);

    // possibly randomize this name at runtime to prevent some attacks?

#    ifndef NDEBUG
    lua_getfield(L, gtable, "load");
    lua_setfield(L, LUA_REGISTRYINDEX, "real_load");
#    endif

    lua_getfield(L, gtable, "dofile");
    lua_setfield(L, LUA_REGISTRYINDEX, "real_dofile");

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    static const luaL_Reg replacementFuncs[] = {
        {"load", lua::api::g_load},
        {"print", lua::api::g_print},

        // This function replaces both `dofile` and `require`, see docs/wip-plugins.md for more info
        {"import", lua::api::g_import},
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
    this->plugins_.insert({pluginName, std::move(plugin)});
    if (!PluginController::isPluginEnabled(pluginName) ||
        !getSettings()->pluginsEnabled)
    {
        qCDebug(chatterinoLua) << "Skipping loading" << pluginName << "("
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

bool PluginController::reload(const QString &id)
{
    auto it = this->plugins_.find(id);
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
    QDir loadDir = it->second->loadDirectory_;
    this->plugins_.erase(id);
    this->tryLoadFromDir(loadDir);
    return true;
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

            auto *L = plugin->state_;
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

bool PluginController::isPluginEnabled(const QString &id)
{
    auto vec = getSettings()->enabledPlugins.getValue();
    auto it = std::find(vec.begin(), vec.end(), id);
    return it != vec.end();
}

Plugin *PluginController::getPluginByStatePtr(lua_State *L)
{
    for (auto &[name, plugin] : this->plugins_)
    {
        if (plugin->state_ == L)
        {
            return plugin.get();
        }
    }
    return nullptr;
}

const std::map<QString, std::unique_ptr<Plugin>> &PluginController::plugins()
    const
{
    return this->plugins_;
}

};  // namespace chatterino
#endif
