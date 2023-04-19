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
    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
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

int c2_send_msg(lua_State *L)
{
    QString text;
    QString channel;
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "send_msg needs exactly 2 arguments (channel and text)");
        lua::push(L, false);
        return 1;
    }
    if (!lua::pop(L, &text))
    {
        luaL_error(
            L, "cannot get text (2nd argument of send_msg, expected a string)");
        lua::push(L, false);
        return 1;
    }
    if (!lua::pop(L, &channel))
    {
        luaL_error(
            L,
            "cannot get channel (1st argument of send_msg, expected a string)");
        lua::push(L, false);
        return 1;
    }

    const auto chn = getApp()->twitch->getChannelOrEmpty(channel);
    if (chn->isEmpty())
    {
        auto *pl = getApp()->plugins->getPluginByStatePtr(L);

        qCWarning(chatterinoLua)
            << "Plugin" << pl->id
            << "tried to send a message (using send_msg) to channel" << channel
            << "which is not known";
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

int c2_system_msg(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        luaL_error(L,
                   "system_msg needs exactly 2 arguments (channel and text)");
        lua::push(L, false);
        return 1;
    }
    QString channel;
    QString text;

    if (!lua::pop(L, &text))
    {
        luaL_error(
            L,
            "cannot get text (2nd argument of system_msg, expected a string)");
        lua::push(L, false);
        return 1;
    }
    if (!lua::pop(L, &channel))
    {
        luaL_error(L, "cannot get channel (1st argument of system_msg, "
                      "expected a string)");
        lua::push(L, false);
        return 1;
    }
    const auto chn = getApp()->twitch->getChannelOrEmpty(channel);
    if (chn->isEmpty())
    {
        auto *pl = getApp()->plugins->getPluginByStatePtr(L);
        qCWarning(chatterinoLua)
            << "Plugin" << pl->id
            << "tried to show a system message (using system_msg) in channel"
            << channel << "which is not known";
        lua::push(L, false);
        return 1;
    }
    chn->addMessage(makeSystemMessage(text));
    lua::push(L, true);
    return 1;
}

int c2_log(lua_State *L)
{
    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
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

int g_import(lua_State *L)
{
    auto countArgs = lua_gettop(L);
    // Lua allows dofile() which loads from stdin, but this is very useless in our case
    if (countArgs == 0)
    {
        lua_pushnil(L);
        luaL_error(L, "it is not allowed to call import() without arguments");
        return 1;
    }

    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
    QString fname;
    if (!lua::pop(L, &fname))
    {
        lua_pushnil(L);
        luaL_error(L, "chatterino g_import: expected a string for a filename");
        return 1;
    }
    auto dir = QUrl(pl->loadDirectory().canonicalPath() + "/");
    auto file = dir.resolved(fname);

    qCDebug(chatterinoLua) << "plugin" << pl->id << "is trying to load" << file
                           << "(its dir is" << dir << ")";
    if (!dir.isParentOf(file))
    {
        lua_pushnil(L);
        luaL_error(L, "chatterino g_import: filename must be inside of the "
                      "plugin directory");
        return 1;
    }

    auto path = file.path(QUrl::FullyDecoded);
    QFile qf(path);
    qf.open(QIODevice::ReadOnly);
    if (qf.size() > 10'000'000)
    {
        lua_pushnil(L);
        luaL_error(L, "chatterino g_import: size limit of 10MB exceeded, what "
                      "the hell are you doing");
        return 1;
    }

    // validate utf-8 to block bytecode exploits
    auto data = qf.readAll();
    auto *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    utf8->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars != 0)
    {
        lua_pushnil(L);
        luaL_error(L, "invalid utf-8 in import() target (%s) is not allowed",
                   fname.toStdString().c_str());
        return 1;
    }

    // fetch dofile and call it
    lua_getfield(L, LUA_REGISTRYINDEX, "real_dofile");
    // maybe data race here if symlink was swapped?
    lua::push(L, path);
    lua_call(L, 1, LUA_MULTRET);

    return lua_gettop(L);
}

int g_print(lua_State *L)
{
    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
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
