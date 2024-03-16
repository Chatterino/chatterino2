#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/LuaAPI.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <lualib.h>
#    include <QFileInfo>
#    include <QLoggingCategory>
#    include <QTextCodec>
#    include <QUrl>

namespace {
using namespace chatterino;

void logHelper(lua_State *L, Plugin *pl, QDebug stream, int argc)
{
    stream.noquote();
    stream << "[" + pl->id + ":" + pl->meta.name + "]";
    for (int i = 1; i <= argc; i++)
    {
        stream << lua::toString(L, i);
    }
    lua_pop(L, argc);
}

QDebug qdebugStreamForLogLevel(lua::api::LogLevel lvl)
{
    auto base =
        (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE,
                        QT_MESSAGELOG_FUNC, chatterinoLua().categoryName()));

    using LogLevel = lua::api::LogLevel;

    switch (lvl)
    {
        case LogLevel::Debug:
            return base.debug();
        case LogLevel::Info:
            return base.info();
        case LogLevel::Warning:
            return base.warning();
        case LogLevel::Critical:
            return base.critical();
        default:
            assert(false && "if this happens magic_enum must have failed us");
            return QDebug((QString *)nullptr);
    }
}

}  // namespace

// NOLINTBEGIN(*vararg)
// luaL_error is a c-style vararg function, this makes clang-tidy not dislike it so much
namespace chatterino::lua::api {

int c2_register_command(lua_State *L)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "internal error: no plugin");
        return 0;
    }

    QString name;
    if (!lua::peek(L, &name, 1))
    {
        luaL_error(L, "cannot get command name (1st arg of register_command, "
                      "expected a string)");
        return 0;
    }
    if (lua_isnoneornil(L, 2))
    {
        luaL_error(L, "missing argument for register_command: function "
                      "\"pointer\"");
        return 0;
    }

    auto callbackSavedName = QString("c2commandcb-%1").arg(name);
    lua_setfield(L, LUA_REGISTRYINDEX, callbackSavedName.toStdString().c_str());
    auto ok = pl->registerCommand(name, callbackSavedName);

    // delete both name and callback
    lua_pop(L, 2);

    lua::push(L, ok);
    return 1;
}

int c2_register_callback(lua_State *L)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "internal error: no plugin");
        return 0;
    }
    EventType evtType{};
    if (!lua::peek(L, &evtType, 1))
    {
        luaL_error(L, "cannot get event name (1st arg of register_callback, "
                      "expected a string)");
        return 0;
    }
    if (lua_isnoneornil(L, 2))
    {
        luaL_error(L, "missing argument for register_callback: function "
                      "\"pointer\"");
        return 0;
    }

    auto callbackSavedName = QString("c2cb-%1").arg(
        magic_enum::enum_name<EventType>(evtType).data());
    lua_setfield(L, LUA_REGISTRYINDEX, callbackSavedName.toStdString().c_str());

    lua_pop(L, 2);

    return 0;
}

int c2_log(lua_State *L)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "c2_log: internal error: no plugin?");
        return 0;
    }
    auto logc = lua_gettop(L) - 1;
    // This is almost the expansion of qCDebug() macro, actual thing is wrapped in a for loop
    LogLevel lvl{};
    if (!lua::pop(L, &lvl, 1))
    {
        luaL_error(L, "Invalid log level, use one from c2.LogLevel.");
        return 0;
    }
    QDebug stream = qdebugStreamForLogLevel(lvl);
    logHelper(L, pl, stream, logc);
    return 0;
}

int c2_later(lua_State *L)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        return luaL_error(L, "c2.later: internal error: no plugin?");
    }
    if (lua_gettop(L) != 2)
    {
        return luaL_error(
            L, "c2.later expects two arguments (a callback that takes no "
               "arguments and returns nothing and a number the time in "
               "milliseconds to wait)\n");
    }
    int time{};
    if (!lua::pop(L, &time))
    {
        return luaL_error(L, "cannot get time (2nd arg of c2.later, "
                             "expected a number)");
    }

    if (!lua_isfunction(L, lua_gettop(L)))
    {
        return luaL_error(L, "cannot get callback (1st arg of c2.later, "
                             "expected a function)");
    }

    auto *timer = new QTimer();
    timer->setInterval(time);
    auto id = pl->addTimeout(timer);
    auto name = QString("timeout_%1").arg(id);
    auto *coro = lua_newthread(L);

    QObject::connect(timer, &QTimer::timeout, [pl, coro, name, timer]() {
        timer->deleteLater();
        pl->removeTimeout(timer);
        int nres{};
        lua_resume(coro, nullptr, 0, &nres);

        lua_pushnil(coro);
        lua_setfield(coro, LUA_REGISTRYINDEX, name.toStdString().c_str());
        if (lua_gettop(coro) != 0)
        {
            stackDump(coro,
                      pl->id +
                          ": timer returned a value, this shouldn't happen "
                          "and is probably a plugin bug");
        }
    });
    stackDump(L, "before setfield");
    lua_setfield(L, LUA_REGISTRYINDEX, name.toStdString().c_str());
    lua_xmove(L, coro, 1);  // move function to thread
    timer->start();

    return 0;
}

int g_load(lua_State *L)
{
#    ifdef NDEBUG
    luaL_error(L, "load() is only usable in debug mode");
    return 0;
#    else
    auto countArgs = lua_gettop(L);
    QByteArray data;
    if (lua::peek(L, &data, 1))
    {
        auto *utf8 = QTextCodec::codecForName("UTF-8");
        QTextCodec::ConverterState state;
        utf8->toUnicode(data.constData(), data.size(), &state);
        if (state.invalidChars != 0)
        {
            luaL_error(L, "invalid utf-8 in load() is not allowed");
            return 0;
        }
    }
    else
    {
        luaL_error(L, "using reader function in load() is not allowed");
        return 0;
    }

    for (int i = 0; i < countArgs; i++)
    {
        lua_seti(L, LUA_REGISTRYINDEX, i);
    }

    // fetch load and call it
    lua_getfield(L, LUA_REGISTRYINDEX, "real_load");

    for (int i = 0; i < countArgs; i++)
    {
        lua_geti(L, LUA_REGISTRYINDEX, i);
        lua_pushnil(L);
        lua_seti(L, LUA_REGISTRYINDEX, i);
    }

    lua_call(L, countArgs, LUA_MULTRET);

    return lua_gettop(L);
#    endif
}

int loadfile(lua_State *L, const QString &str)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        return luaL_error(L, "loadfile: internal error: no plugin?");
    }
    auto dir = QUrl(pl->loadDirectory().canonicalPath() + "/");

    if (!dir.isParentOf(str))
    {
        // XXX: This intentionally hides the resolved path to not leak it
        lua::push(
            L, QString("requested module is outside of the plugin directory"));
        return 1;
    }
    auto datadir = QUrl(pl->dataDirectory().canonicalPath() + "/");
    if (datadir.isParentOf(str))
    {
        lua::push(L, QString("requested file is data, not code, see Chatterino "
                             "documentation"));
        return 1;
    }

    QFileInfo info(str);
    if (!info.exists())
    {
        lua::push(L, QString("no file '%1'").arg(str));
        return 1;
    }

    auto temp = str.toStdString();
    const auto *filename = temp.c_str();

    auto res = luaL_loadfilex(L, filename, "t");
    // Yoinked from checkload lib/lua/src/loadlib.c
    if (res == LUA_OK)
    {
        lua_pushstring(L, filename);
        return 2;
    }

    return luaL_error(L, "error loading module '%s' from file '%s':\n\t%s",
                      lua_tostring(L, 1), filename, lua_tostring(L, -1));
}

int searcherAbsolute(lua_State *L)
{
    auto name = QString::fromUtf8(luaL_checkstring(L, 1));
    name = name.replace('.', QDir::separator());

    QString filename;
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        return luaL_error(L, "searcherAbsolute: internal error: no plugin?");
    }

    QFileInfo file(pl->loadDirectory().filePath(name + ".lua"));
    return loadfile(L, file.canonicalFilePath());
}

int searcherRelative(lua_State *L)
{
    lua_Debug dbg;
    lua_getstack(L, 1, &dbg);
    lua_getinfo(L, "S", &dbg);
    auto currentFile = QString::fromUtf8(dbg.source, dbg.srclen);
    if (currentFile.startsWith("@"))
    {
        currentFile = currentFile.mid(1);
    }
    if (currentFile == "=[C]" || currentFile == "")
    {
        lua::push(
            L,
            QString(
                "Unable to load relative to file:caller has no source file"));
        return 1;
    }

    auto parent = QFileInfo(currentFile).dir();

    auto name = QString::fromUtf8(luaL_checkstring(L, 1));
    name = name.replace('.', QDir::separator());
    QString filename =
        parent.canonicalPath() + QDir::separator() + name + ".lua";

    return loadfile(L, filename);
}

int g_print(lua_State *L)
{
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "c2_print: internal error: no plugin?");
        return 0;
    }
    auto argc = lua_gettop(L);
    // This is almost the expansion of qCDebug() macro, actual thing is wrapped in a for loop
    auto stream =
        (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE,
                        QT_MESSAGELOG_FUNC, chatterinoLua().categoryName())
             .debug());
    logHelper(L, pl, stream, argc);
    return 0;
}

}  // namespace chatterino::lua::api
// NOLINTEND(*vararg)
#endif
