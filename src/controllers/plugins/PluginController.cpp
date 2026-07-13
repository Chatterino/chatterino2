// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginController.hpp"

#    include "Application.hpp"
#    include "common/Args.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/api/Accounts.hpp"
#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/api/ConnectionHandle.hpp"
#    include "controllers/plugins/api/DebugLibrary.hpp"
#    include "controllers/plugins/api/HTTPRequest.hpp"
#    include "controllers/plugins/api/HTTPResponse.hpp"
#    include "controllers/plugins/api/Images.hpp"
#    include "controllers/plugins/api/IOWrapper.hpp"
#    include "controllers/plugins/api/JSON.hpp"
#    include "controllers/plugins/api/Message.hpp"
#    include "controllers/plugins/api/WebSocket.hpp"
#    include "controllers/plugins/api/WindowManager.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "messages/MessageElement.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"
#    include "singletons/WindowManager.hpp"
#    include "util/FilesystemHelpers.hpp"
#    include "util/PostToThread.hpp"
#    include "widgets/splits/SplitContainer.hpp"
#    include "widgets/Window.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <lualib.h>
#    include <QJsonDocument>
#    include <sol/overload.hpp>
#    include <sol/sol.hpp>
#    include <sol/types.hpp>
#    include <sol/variadic_args.hpp>
#    include <sol/variadic_results.hpp>

#    include <memory>
#    include <utility>
#    include <variant>

namespace chatterino {

using namespace Qt::Literals;

PluginController::PluginController(const Paths &paths_)
    : paths(paths_)
    , lifetime(std::make_shared<bool>(true))
{
    this->loaders_.emplace_back("chatterino.json", &lua::api::loadJson);
}

void PluginController::initialize(Settings &settings)
{
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
    auto dir = QDir(this->paths.pluginsDirectory);
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
    if (!infoFile.open(QIODevice::ReadOnly))
    {
        qCWarning(chatterinoLua)
            << "Could not open info.json" << infoFile.errorString();
        return false;
    }
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

void PluginController::openLibrariesFor(Plugin *plugin)
{
    auto *L = plugin->state_;
    lua::StackGuard guard(L);
    sol::state_view lua(L);
    // Stuff to change, remove or hide behind a permission system:
    static const std::vector<luaL_Reg> loadedlibs = {
        luaL_Reg{LUA_GNAME, luaopen_base},
        // - load - don't allow in release mode

        luaL_Reg{LUA_COLIBNAME, luaopen_coroutine},
        luaL_Reg{LUA_TABLIBNAME, luaopen_table},
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
    luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, int(false));
    lua_setfield(L, LUA_REGISTRYINDEX, lua::api::REG_REAL_IO_NAME);

    auto r = lua.registry();
    auto g = lua.globals();
    auto c2 = lua.create_table();
    g["c2"] = c2;

    // ban functions
    // Note: this might not be fully secure? some kind of metatable fuckery might come up?

#    ifndef NDEBUG
    lua.registry()["real_load"] = lua.globals()["load"];
#    endif
    // See chatterino::lua::api::g_load implementation

    g["loadfile"] = sol::nil;
    g["dofile"] = sol::nil;

    // set up package lib
    {
        auto package = g["package"];
        package["cpath"] = "";
        package["path"] = "";

        sol::protected_function tbremove = g["table"]["remove"];

        // remove searcher_Croot, searcher_C and searcher_Lua leaving only searcher_preload
        sol::table searchers = package["searchers"];
        for (int i = 0; i < 3; i++)
        {
            tbremove(searchers);
        }
        searchers.add(&lua::api::searcherRelative);
        searchers.add(&lua::api::searcherAbsolute);
    }
    // set up io lib
    {
        auto c2io = lua.create_table();
        auto realio = r[lua::api::REG_REAL_IO_NAME];
        c2io["type"] = realio["type"];
        g["io"] = c2io;
        // prevent plugins getting direct access to realio
        r[LUA_LOADED_TABLE]["io"] = c2io;

        // Don't give plugins the option to shit into our stdio
        r["_IO_input"] = sol::nil;
        r["_IO_output"] = sol::nil;
    }
    // set up debug lib
    {
        auto debuglib = lua.create_table();
        g["debug"] = debuglib;

        debuglib.set_function("traceback", lua::api::debugTraceback);
    }
    PluginController::initSol(lua, plugin);
}

// TODO: investigate if `plugin` can ever point to an invalid plugin,
// especially in cases when the plugin is errored.
void PluginController::initSol(sol::state_view &lua, Plugin *plugin)
{
    auto g = lua.globals();
    // Do not capture plugin->state_ in lambdas, this makes the functions unusable in callbacks
    g.set_function("print", &lua::api::g_print);
    g.set_function("load", &lua::api::g_load);

    sol::table c2 = g["c2"];
    c2.set_function("register_command",
                    [plugin](const QString &name, sol::protected_function cb) {
                        return plugin->registerCommand(name, std::move(cb));
                    });
    c2.set_function("register_callback", &lua::api::c2_register_callback);
    c2.set_function("log", &lua::api::c2_log);
    c2.set_function("later", &lua::api::c2_later);

    lua::api::ChannelRef::createUserType(c2);
    lua::api::HTTPResponse::createUserType(c2);
    lua::api::HTTPRequest::createUserType(c2);
    lua::api::WebSocket::createUserType(c2, plugin);
    lua::api::ConnectionHandle::createUserType(c2);
    lua::api::message::createUserType(c2);
    lua::api::images::createUserTypes(c2);
    lua::api::createAccounts(c2);
    lua::api::windowmanager::createUserTypes(c2);
    c2["ChannelType"] = lua::createEnumTable<Channel::Type>(lua);
    c2["HTTPMethod"] = lua::createEnumTable<NetworkRequestType>(lua);
    c2["EventType"] = lua::createEnumTable<lua::api::EventType>(lua);
    c2["LogLevel"] = lua::createEnumTable<lua::api::LogLevel>(lua);
    c2["MessageFlag"] =
        lua::createEnumTable<MessageFlag, MessageFlag::None>(lua);
    c2["MessageElementFlag"] = lua::createEnumTable<MessageElementFlag>(lua);
    c2["FontStyle"] = lua::createEnumTable<FontStyle>(lua);
    c2["MessageContext"] = lua::createEnumTable<MessageContext>(lua);
    c2["LinkType"] =
        lua::createEnumTable<lua::api::message::ExposedLinkType>(lua);
    c2["SplitContainerNodeType"] =
        lua::createEnumTable<SplitContainer::Node::Type>(lua);
    c2["WindowType"] = lua::createEnumTable<WindowType>(lua);

    c2["windows"] = getApp()->getWindows();

    sol::table io = g["io"];
    io.set_function(
        "open", sol::overload(&lua::api::io_open, &lua::api::io_open_modeless));
    io.set_function("lines", sol::overload(&lua::api::io_lines,
                                           &lua::api::io_lines_noargs));
    io.set_function("input", sol::overload(&lua::api::io_input_argless,
                                           &lua::api::io_input_name,
                                           &lua::api::io_input_file));
    io.set_function("output", sol::overload(&lua::api::io_output_argless,
                                            &lua::api::io_output_name,
                                            &lua::api::io_output_file));
    io.set_function("close", sol::overload(&lua::api::io_close_argless,
                                           &lua::api::io_close_file));
    io.set_function("flush", sol::overload(&lua::api::io_flush_argless,
                                           &lua::api::io_flush_file));
    io.set_function("read", &lua::api::io_read);
    io.set_function("write", &lua::api::io_write);
    io.set_function("popen", &lua::api::io_popen);
    io.set_function("tmpfile", &lua::api::io_tmpfile);

    sol::table package = g["package"];
    package.set_function("loadlib", &lua::api::package_loadlib);

    for (const auto &[name, fn] : this->loaders_)
    {
        package["preload"][name] = [fn](sol::this_main_state state) {
            sol::state_view sv(state);
            return fn(sv);
        };
    }
}

void PluginController::load(const QFileInfo &index, const QDir &pluginDir,
                            const PluginMeta &meta)
{
    auto pluginName = pluginDir.dirName();
    lua_State *l = luaL_newstate();
    auto plugin = std::make_unique<Plugin>(pluginName, l, meta, pluginDir);
    auto *temp = plugin.get();
    this->plugins_.insert({pluginName, std::move(plugin)});
    this->queueChangeNotification();

    if (getApp()->getArgs().safeMode)
    {
        // This isn't done earlier to ensure the user can disable a misbehaving plugin
        qCWarning(chatterinoLua) << "Skipping loading plugin " << meta.name
                                 << " because safe mode is enabled.";
        return;
    }
    this->openLibrariesFor(temp);

    if (!PluginController::isPluginEnabled(pluginName) ||
        !getSettings()->pluginsEnabled)
    {
        qCDebug(chatterinoLua) << "Skipping loading" << pluginName << "("
                               << meta.name << ") because it is disabled";
        return;
    }
    temp->dataDirectory().mkpath(".");

    // make sure we capture log messages during load
    this->onPluginLoaded.invoke(temp);
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

    for (const auto &[cmd, _] : it->second->ownedCommands)
    {
        getApp()->getCommands()->unregisterPluginCommand(cmd);
    }
    QDir loadDir = it->second->loadDirectory_;
    // Since Plugin owns the state, it will clean up everything related to it
    this->plugins_.erase(id);
    this->queueChangeNotification();
    this->tryLoadFromDir(loadDir);
    return true;
}

ExpectedStr<void> PluginController::removePlugin(const QString &id,
                                                 RemovePluginArgs args)
{
    auto it = this->plugins_.find(id);
    if (it == this->plugins_.end())
    {
        return makeUnexpected("Plugin not found");
    }

    QDir loadDirectory = it->second->loadDirectory();
    this->plugins_.erase(it);
    this->queueChangeNotification();

    if (args.eraseData)
    {
        qCDebug(chatterinoLua)
            << "Recursively removing" << loadDirectory.absolutePath();
        if (!loadDirectory.removeRecursively())
        {
            return makeUnexpected(u"Failed to remove directory"_s);
        }
    }
    else
    {
        const auto items = loadDirectory.entryInfoList(QDir::AllEntries |
                                                       QDir::NoDotAndDotDot);
        for (const auto &entry : items)
        {
            if (entry.fileName() == "data")
            {
                continue;
            }

            qCDebug(chatterinoLua) << "Removing" << entry.absoluteFilePath();

            bool ok = false;
            if (entry.isDir())
            {
                ok = QDir(entry.absoluteFilePath()).removeRecursively();
            }
            else
            {
                ok = loadDirectory.remove(entry.fileName());
            }

            if (!ok)
            {
                return makeUnexpected(u"Failed to remove '" % entry.fileName() %
                                      "'");
            }
        }
    }

    if (args.disable)
    {
        auto vec = getSettings()->enabledPlugins.getValue();
        vec.removeAll(id);
        getSettings()->enabledPlugins.setValue(vec);
    }

    return {};
}

void PluginController::download(const DownloadArgs &args)
{
    auto id = args.remotePlugin->id;
    auto existingIt = this->plugins_.find(id);
    if (existingIt != this->plugins_.end())
    {
        if (args.update)
        {
            QString conflicts;
            if (!existingIt->second->meta.isRelatedTo(args.remotePlugin->meta,
                                                      &conflicts))
            {
                args.onDone(makeUnexpected(
                    u"Plugin was installed from a different source: " %
                    conflicts));
                return;
            }
        }
        else if (args.onExistingOverwrite)
        {
            if (!args.onExistingOverwrite())
            {
                // The user should know about it - don't show a new error or success.
                return;
            }
        }
        else
        {
            args.onDone(makeUnexpected(u"Plugin already exists"_s));
            return;
        }

        auto err = this->removePlugin(id, {
                                              .eraseData = false,
                                              .disable = false,
                                          });
        if (!err)
        {
            args.onDone(err);
            return;
        }
    }
    else if (args.update)
    {
        args.onDone(makeUnexpected(u"Plugin does not exist."_s));
        return;
    }

    auto pluginDir =
        qStringToStdPath(this->paths.pluginsDirectory) / qStringToStdPath(id);
    std::error_code ec;
    std::filesystem::create_directories(pluginDir, ec);
    if (ec)
    {
        args.onDone(makeUnexpected(u"Failed to create plugin directory: " %
                                   QString::fromStdString(ec.message())));
        return;
    }

    pluginDir = std::filesystem::canonical(pluginDir, ec);
    if (ec)
    {
        args.onDone(makeUnexpected(u"Failed canonicalize plugin directory: " %
                                   QString::fromStdString(ec.message())));
        return;
    }
    auto infoJsonPath = pluginDir / "info.json";

    QUrl baseUrl = args.remotePlugin->downloadURL;
    std::vector<std::pair<QUrl, QString>> files;
    for (const auto &path : args.remotePlugin->meta.files)
    {
        auto url = baseUrl;
        url.setPath(url.path(QUrl::FullyEncoded) % '/' % path);
        if (!baseUrl.isParentOf(url))
        {
            args.onDone(makeUnexpected(u"Plugin contains invalid file: '" %
                                       url.toString() % "'"));
            return;
        }
        auto localPath = pluginDir / qStringToStdPath(path);
        if (localPath == infoJsonPath)
        {
            continue;  // We write this later.
        }
        auto parent = localPath.parent_path();
        if (parent != localPath)
        {
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                args.onDone(makeUnexpected(
                    u"Failed crete parent directories for '" % path % u"': " %
                    QString::fromStdString(ec.message())));
                return;
            }
        }
        files.emplace_back(std::move(url), stdPathToQString(localPath));
    }

    if (files.empty())
    {
        args.onDone(makeUnexpected(u"No files to download"_s));
        return;
    }

    struct State {
        RemotePluginPtr plugin;
        std::filesystem::path pluginDir;

        size_t remainingRequests = 0;
        QString errors;

        std::function<void(ExpectedStr<void>)> onDone;
    };
    auto state = std::make_shared<State>(State{
        .plugin = args.remotePlugin,
        .pluginDir = pluginDir,
        .remainingRequests = files.size(),
        .onDone = args.onDone,
    });

    auto pushResult = [this, lifetime = std::weak_ptr{this->lifetime},
                       state](ExpectedStr<void> res) {
        if (lifetime.expired())
        {
            return;
        }

        assertInGuiThread();
        if (!res)
        {
            if (!state->errors.isEmpty())
            {
                state->errors += u"\n";
            }
            state->errors += res.error();
        }
        state->remainingRequests -= 1;
        if (state->remainingRequests > 0)
        {
            return;
        }

        // We're the last result everyone was waiting for.
        if (!state->errors.isEmpty())
        {
            state->onDone(makeUnexpected(state->errors));
            return;
        }

        state->onDone(this->finishDownload(*state->plugin, state->pluginDir));
    };
    auto pushOnGUI = [pushResult](ExpectedStr<void> res) {
        runInGuiThread([pushResult, res = std::move(res)] mutable {
            pushResult(std::move(res));
        });
    };

    auto fetchFile = args.fetchFile;
    if (!fetchFile)
    {
        fetchFile =
            +[](const QUrl &url,
                const std::function<void(ExpectedStr<QByteArray>)> &cb) {
                NetworkRequest(url)
                    .timeout(30'000)
                    .concurrent()
                    .onSuccess([cb](const NetworkResult &res) {
                        cb(res.getData());
                    })
                    .onError([cb, url](const NetworkResult &res) {
                        cb(makeUnexpected(u"Failed to fetch '" %
                                          url.toString() % u"': " %
                                          res.formatError()));
                    })
                    .execute();
            };
    }

    // Send out the requests - rely on Qt to buffer requests.
    for (const auto &[url, path] : files)
    {
        fetchFile(url, [pushOnGUI, path](ExpectedStr<QByteArray> res) mutable {
            if (!res)
            {
                pushOnGUI(makeUnexpected(std::move(res).error()));
                return;
            }

            {
                QFile f(path);
                if (!f.open(QFile::WriteOnly | QFile::Truncate))
                {
                    pushOnGUI(makeUnexpected(u"Failed to open '" % path %
                                             u"' for writing: " %
                                             f.errorString()));
                    return;
                }
                f.write(*res);
            }
            pushOnGUI({});
        });
    }
}

ExpectedStr<void> PluginController::finishDownload(
    const RemotePlugin &remote, const std::filesystem::path &pluginDir)
{
    {
        QFile metaFile(stdPathToQString(pluginDir / "info.json"));
        if (!metaFile.open(QFile::WriteOnly))
        {
            return makeUnexpected(u"Failed to open info.json for writing: " %
                                  metaFile.errorString());
        }
        metaFile.write(QJsonDocument(remote.meta.toJson()).toJson());
    }
    bool ok = this->tryLoadFromDir(pluginDir);
    if (!ok)
    {
        return makeUnexpected(u"Failed to load plugin from directory"_s);
    }

    return {};
}

QString PluginController::tryExecPluginCommand(const QString &commandName,
                                               const CommandContext &ctx)
{
    for (auto &[name, plugin] : this->plugins_)
    {
        if (auto it = plugin->ownedCommands.find(commandName);
            it != plugin->ownedCommands.end())
        {
            sol::state_view lua(plugin->state_);
            sol::table args = lua.create_table_with(
                "words", ctx.words,                           //
                "channel", lua::api::ChannelRef(ctx.channel)  //
            );

            auto result =
                lua::tryCall<std::optional<QString>>(it->second, args);
            if (!result)
            {
                ctx.channel->addSystemMessage(
                    QStringView(
                        u"Failed to evaluate command from plugin %1: %2")
                        .arg(plugin->meta.name, result.error()));
                return {};
            }

            auto opt = result.value();
            if (!opt)
            {
                return {};
            }
            return *opt;
        }
    }
    qCCritical(chatterinoLua)
        << "Something's seriously up, no plugin owns command" << commandName
        << "yet a call to execute it came in";
    assert(false && "missing plugin command owner");
    return {};
}

bool PluginController::isPluginEnabled(const QString &id)
{
    return getSettings()->enabledPlugins.getValue().contains(id);
}

Plugin *PluginController::getPluginByStatePtr(lua_State *L)
{
    lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    // Use the main thread for identification, not a coroutine instance
    auto *mainL = lua_tothread(L, -1);
    lua_pop(L, 1);
    L = mainL;
    for (auto &[name, plugin] : this->plugins_)
    {
        if (plugin->state_ == L)
        {
            return plugin.get();
        }
    }
    return nullptr;
}

Plugin *PluginController::getPluginByID(const QString &id)
{
    auto it = this->plugins_.find(id);
    if (it != this->plugins_.end())
    {
        return it->second.get();
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
        if (!pl->error().isNull() || pl->state_ == nullptr)
        {
            continue;
        }

        auto opt = pl->getCompletionCallback();
        if (opt)
        {
            qCDebug(chatterinoLua)
                << "Processing custom completions from plugin" << name;
            auto &cb = *opt;
            sol::state_view view(pl->state_);
            auto errOrList = lua::tryCall<sol::table>(
                cb,
                toTable(pl->state_, lua::api::CompletionEvent{
                                        .query = query,
                                        .full_text_content = fullTextContent,
                                        .cursor_position = cursorPosition,
                                        .is_first_word = isFirstWord,
                                    }));
            if (!errOrList.has_value())
            {
                qCDebug(chatterinoLua)
                    << "Got error from plugin " << pl->meta.name
                    << " while refreshing tab completion: "
                    << errOrList.error();
                continue;
            }

            auto list = lua::api::CompletionList(*errOrList);
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

WebSocketPool &PluginController::webSocketPool()
{
    return this->webSocketPool_;
}

void PluginController::queueChangeNotification()
{
    if (this->changeNotificationQueued)
    {
        return;
    }
    this->changeNotificationQueued = true;
    QMetaObject::invokeMethod(
        qApp,
        [this, weak = std::weak_ptr(this->lifetime)] {
            if (!weak.lock())
            {
                return;
            }
            this->changeNotificationQueued = false;
            this->onPluginsUpdated.invoke();
        },
        Qt::QueuedConnection);
}

}  // namespace chatterino
#endif
