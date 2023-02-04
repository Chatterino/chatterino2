#ifdef CHATTERINO_HAVE_PLUGINS
#    include "LuaApi.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "messages/MessageBuilder.hpp"
#    include "providers/twitch/TwitchIrcServer.hpp"

// lua stuff
#    include "lauxlib.h"
#    include "lua.h"
#    include "lualib.h"

#    include <QFileInfo>
#    include <QTextCodec>
namespace chatterino::lua::api {

int c2_register_command(lua_State *L)
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

int c2_system_msg(lua_State *L)
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

static const QChar REPLACEMENT_CHARACTER = QChar(0xFFFD);
int g_load(lua_State *L)
{
    auto countArgs = lua_gettop(L);
    QByteArray data;
    if (lua::peek(L, &data, 1))
    {
        auto *utf8 = QTextCodec::codecForName("UTF-8");
        QTextCodec::ConverterState state;
        utf8->toUnicode(data.constData(), data.size(), &state);
        if (state.invalidChars != 0)
        {
            // NOLINTNEXTLINE
            luaL_error(L, "invalid utf-8 in load() is not allowed");
            return 0;
        }
    }
    else
    {
        // NOLINTNEXTLINE
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
}

int g_dofile(lua_State *L)
{
    auto countArgs = lua_gettop(L);
    // Lua allows dofile() which loads from stdin, but this is very useless in our case
    if (countArgs == 0)
    {
        lua_pushnil(L);
        // NOLINTNEXTLINE
        luaL_error(L, "it is not allowed to call dofile() without arguments");
        return 1;
    }

    auto *pl = getApp()->plugins->getPluginByStatePtr(L);
    QString fname;
    if (!lua::pop(L, &fname))
    {
        lua_pushnil(L);
        // NOLINTNEXTLINE
        luaL_error(L, "chatterino g_dofile: expected a string for a filename");
        return 1;
    }
    auto dir = QUrl(pl->loadDirectory().canonicalPath() + "/");
    auto file = dir.resolved(fname);

    qCDebug(chatterinoLua) << "plugin" << pl->codename << "is trying to load"
                           << file << "(its dir is" << dir << ")";
    if (!dir.isParentOf(file))
    {
        lua_pushnil(L);
        // NOLINTNEXTLINE
        luaL_error(L, "chatterino g_dofile: filename must be inside of the "
                      "plugin directory");
        return 1;
    }

    auto path = file.path(QUrl::FullyDecoded);
    // validate utf-8 to block bytecode exploits
    QFile qf(path);
    qf.open(QIODevice::ReadOnly);
    if (qf.size() > 10'000'000)
    {
        lua_pushnil(L);
        // NOLINTNEXTLINE
        luaL_error(L, "chatterino g_dofile: size limit of 10MB exceeded, what "
                      "the hell are you doing");
        return 1;
    }
    auto data = qf.readAll();
    auto *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    utf8->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars != 0)
    {
        lua_pushnil(L);
        // NOLINTNEXTLINE
        luaL_error(L, "invalid utf-8 in dofile() target (%s) is not allowed",
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

}  // namespace chatterino::lua::api
#endif
