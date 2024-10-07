#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/LuaAPI.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // for lua operations on QString{,List} for CompletionList

#    include <lauxlib.h>
#    include <lua.h>
#    include <lualib.h>
#    include <QFileInfo>
#    include <QList>
#    include <QLoggingCategory>
#    include <QTextCodec>
#    include <QUrl>
#    include <sol/forward.hpp>
#    include <sol/state_view.hpp>
#    include <sol/types.hpp>
#    include <sol/variadic_args.hpp>
#    include <sol/variadic_results.hpp>

#    include <string>
#    include <utility>

namespace {
using namespace chatterino;

void logHelper(lua_State *L, Plugin *pl, QDebug stream,
               const sol::variadic_args &args)
{
    stream.noquote();
    stream << "[" + pl->id + ":" + pl->meta.name + "]";
    for (const auto &arg : args)
    {
        stream << lua::toString(L, arg.stack_index());
        // Remove this from our stack
        lua_pop(L, 1);
    }
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

CompletionList::CompletionList(const sol::table &table)
    : values(table.get<QStringList>("values"))
    , hideOthers(table["hide_others"])
{
}

sol::table toTable(lua_State *L, const CompletionEvent &ev)
{
    return sol::state_view(L).create_table_with(
        "query", ev.query,                          //
        "full_text_content", ev.full_text_content,  //
        "cursor_position", ev.cursor_position,      //
        "is_first_word", ev.is_first_word           //
    );
}

void c2_register_callback(Plugin *pl, EventType evtType,
                          sol::protected_function callback)
{
    pl->callbacks[evtType] = std::move(callback);
}

void c2_log(sol::this_state L, Plugin *pl, LogLevel lvl,
            sol::variadic_args args)
{
    lua::StackGuard guard(L);
    {
        QDebug stream = qdebugStreamForLogLevel(lvl);
        logHelper(L, pl, stream, args);
    }
}

int c2_later(lua_State *L)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
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

sol::variadic_results g_load(sol::this_state s, sol::object data)
{
#    ifdef NDEBUG
    throw std::runtime_error("load() is only usable in debug mode");
#    else

    // If you're modifying this PLEASE verify it works, Sol is very annoying about serialization
    // - Mm2PL
    sol::state_view lua(s);
    auto load = lua.registry()["real_load"];
    sol::protected_function_result ret = load(data, "=(load)", "t");
    return ret;
#    endif
}

int loadfile(lua_State *L, const QString &str)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
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
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
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
    lua_getstack(L, 2, &dbg);
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

void g_print(sol::this_state L, Plugin *pl, sol::variadic_args args)
{
    // This is almost the expansion of qCDebug() macro, actual thing is wrapped in a for loop
    auto stream =
        (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE,
                        QT_MESSAGELOG_FUNC, chatterinoLua().categoryName())
             .debug());
    logHelper(L, pl, stream, args);
}

}  // namespace chatterino::lua::api
// NOLINTEND(*vararg)
#endif
