#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginController.hpp"

#    include "Application.hpp"
#    include "common/Args.hpp"
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
#    include <variant>

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
         dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        auto pluginDir = QDir(info.absoluteFilePath());
        this->tryLoadFromDir(pluginDir);
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

void PluginController::openLibrariesFor(lua_State *L, const PluginMeta &meta,
                                        const QDir &pluginDir)
{
    lua::StackGuard guard(L);
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
        luaL_Reg{LUA_LOADLIBNAME, luaopen_package},
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
        {"register_callback", lua::api::c2_register_callback},
        {"send_msg", lua::api::c2_send_msg},
        {"log", lua::api::c2_log},
        {nullptr, nullptr},
    };
    lua_pushglobaltable(L);
    auto gtable = lua_gettop(L);

    // count of elements in C2LIB + LogLevel + EventType
    auto c2libIdx = lua::pushEmptyTable(L, 8);

    luaL_setfuncs(L, c2Lib, 0);

    lua::pushEnumTable<lua::api::LogLevel>(L);
    lua_setfield(L, c2libIdx, "LogLevel");

    lua::pushEnumTable<lua::api::EventType>(L);
    lua_setfield(L, c2libIdx, "EventType");

    lua_setfield(L, gtable, "c2");

    // ban functions
    // Note: this might not be fully secure? some kind of metatable fuckery might come up?

    // possibly randomize this name at runtime to prevent some attacks?

#    ifndef NDEBUG
    lua_getfield(L, gtable, "load");
    lua_setfield(L, LUA_REGISTRYINDEX, "real_load");
#    endif

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    static const luaL_Reg replacementFuncs[] = {
        {"load", lua::api::g_load},
        {"print", lua::api::g_print},
        {nullptr, nullptr},
    };
    luaL_setfuncs(L, replacementFuncs, 0);

    lua_pushnil(L);
    lua_setfield(L, gtable, "loadfile");

    lua_pushnil(L);
    lua_setfield(L, gtable, "dofile");

    // set up package lib
    lua_getfield(L, gtable, "package");

    auto package = lua_gettop(L);
    lua_pushstring(L, "");
    lua_setfield(L, package, "cpath");

    // we don't use path
    lua_pushstring(L, "");
    lua_setfield(L, package, "path");

    {
        lua_getfield(L, gtable, "table");
        auto table = lua_gettop(L);
        lua_getfield(L, -1, "remove");
        lua_remove(L, table);
    }
    auto remove = lua_gettop(L);

    // remove searcher_Croot, searcher_C and searcher_Lua leaving only searcher_preload
    for (int i = 0; i < 3; i++)
    {
        lua_pushvalue(L, remove);
        lua_getfield(L, package, "searchers");
        lua_pcall(L, 1, 0, 0);
    }
    lua_pop(L, 1);  // get rid of remove

    lua_getfield(L, package, "searchers");
    lua_pushcclosure(L, lua::api::searcherRelative, 0);
    lua_seti(L, -2, 2);

    lua::push(L, QString(pluginDir.absolutePath()));
    lua_pushcclosure(L, lua::api::searcherAbsolute, 1);
    lua_seti(L, -2, 3);

    lua_pop(L, 3);  // remove gtable, package, package.searchers
}

void PluginController::load(const QFileInfo &index, const QDir &pluginDir,
                            const PluginMeta &meta)
{
    auto pluginName = pluginDir.dirName();
    lua_State *l = luaL_newstate();
    auto plugin = std::make_unique<Plugin>(pluginName, l, meta, pluginDir);
    auto *temp = plugin.get();
    this->plugins_.insert({pluginName, std::move(plugin)});

    if (getApp()->getArgs().safeMode)
    {
        // This isn't done earlier to ensure the user can disable a misbehaving plugin
        qCWarning(chatterinoLua) << "Skipping loading plugin " << meta.name
                                 << " because safe mode is enabled.";
        return;
    }
    PluginController::openLibrariesFor(l, meta, pluginDir);

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
        temp->error_ = lua::humanErrorText(l, err);
        qCWarning(chatterinoLua)
            << "Failed to load" << pluginName << "plugin from" << index << ": "
            << temp->error_;
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

std::pair<bool, QStringList> PluginController::updateCustomCompletions(
    const QString &query, const QString &fullTextContent, int cursorPosition,
    bool isFirstWord) const
{
    QStringList results;

    for (const auto &[name, pl] : this->plugins())
    {
        if (!pl->error().isNull())
        {
            continue;
        }

        lua::StackGuard guard(pl->state_);

        auto opt = pl->getCompletionCallback();
        if (opt)
        {
            qCDebug(chatterinoLua)
                << "Processing custom completions from plugin" << name;
            auto &cb = *opt;
            auto errOrList =
                cb(query, fullTextContent, cursorPosition, isFirstWord);
            if (std::holds_alternative<int>(errOrList))
            {
                guard.handled();
                int err = std::get<int>(errOrList);
                qCDebug(chatterinoLua)
                    << "Got error from plugin " << pl->meta.name
                    << " while refreshing tab completion: "
                    << lua::humanErrorText(pl->state_, err);
                continue;
            }

            auto list = std::get<lua::api::CompletionList>(errOrList);
            if (list.hideOthers)
            {
                results = QStringList(list.values.begin(), list.values.end());
                return {true, results};
            }
            results += QStringList(list.values.begin(), list.values.end());
        }
    }

    return {false, results};
}

}  // namespace chatterino
#endif
