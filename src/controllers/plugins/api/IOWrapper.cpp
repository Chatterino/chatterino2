#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/IOWrapper.hpp"

#    include "Application.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"

extern "C" {
#    include <lauxlib.h>
#    include <lua.h>
}

#    include <cerrno>

namespace chatterino::lua::api {

// Note: Parsing and then serializing the mode ensures we understand it before
// passing it to Lua

struct LuaFileMode {
    char major = 'r';  // 'r'|'w'|'a'
    bool update{};     // '+'
    bool binary{};     // 'b'
    QString error;

    LuaFileMode() = default;

    LuaFileMode(const QString &smode)
    {
        if (smode.isEmpty())
        {
            this->error = "Empty mode given, use one matching /[rwa][+]?b?/.";
            return;
        }
        auto major = smode.at(0);
        if (major != 'r' && major != 'w' && major != 'a')
        {
            this->error = "Invalid mode, use one matching /[rwa][+]?b?/. "
                          "Parsing failed at 1st character.";
            return;
        }
        this->major = major.toLatin1();
        if (smode.length() > 1)
        {
            auto plusOrB = smode.at(1);
            if (plusOrB == '+')
            {
                this->update = true;
            }
            else if (plusOrB == 'b')
            {
                this->binary = true;
            }
            else
            {
                this->error = "Invalid mode, use one matching /[rwa][+]?b?/. "
                              "Parsing failed at 2nd character.";
                return;
            }
        }
        if (smode.length() > 2)
        {
            auto maybeB = smode.at(2);
            if (maybeB == 'b')
            {
                this->binary = true;
            }
            else
            {
                this->error = "Invalid mode, use one matching /[rwa][+]?b?/. "
                              "Parsing failed at 3rd character.";
                return;
            }
        }
    }

    QString toString() const
    {
        assert(this->major == 'r' || this->major == 'w' || this->major == 'a');
        QString out;
        out += this->major;
        if (this->update)
        {
            out += '+';
        }
        if (this->binary)
        {
            out += 'b';
        }
        return out;
    }
};

int ioError(lua_State *L, const QString &value, int errnoequiv)
{
    lua_pushnil(L);
    lua::push(L, value);
    lua::push(L, errnoequiv);
    return 3;
}

// NOLINTBEGIN(*vararg)
int io_open(lua_State *L)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "internal error: no plugin");
        return 0;
    }
    LuaFileMode mode;
    if (lua_gettop(L) == 2)
    {
        // we have a mode
        QString smode;
        if (!lua::pop(L, &smode))
        {
            return luaL_error(
                L,
                "io.open mode (2nd argument) must be a string or not present");
        }
        mode = LuaFileMode(smode);
        if (!mode.error.isEmpty())
        {
            return luaL_error(L, mode.error.toStdString().c_str());
        }
    }
    QString filename;
    if (!lua::pop(L, &filename))
    {
        return luaL_error(L,
                          "io.open filename (1st argument) must be a string");
    }
    QFileInfo file(pl->dataDirectory().filePath(filename));
    auto abs = file.absoluteFilePath();
    qCDebug(chatterinoLua) << "[" << pl->id << ":" << pl->meta.name
                           << "] Plugin is opening file at " << abs
                           << " with mode " << mode.toString();
    bool ok = pl->hasFSPermissionFor(
        mode.update || mode.major == 'w' || mode.major == 'a', abs);
    if (!ok)
    {
        return ioError(L,
                       "Plugin does not have permissions to access given file.",
                       EACCES);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, REG_REAL_IO_NAME);
    lua_getfield(L, -1, "open");
    lua_remove(L, -2);  // remove LUA_REGISTRYINDEX[REAL_IO_NAME]
    lua::push(L, abs);
    lua::push(L, mode.toString());
    lua_call(L, 2, 3);
    return 3;
}

int io_lines(lua_State *L)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        luaL_error(L, "internal error: no plugin");
        return 0;
    }
    if (lua_gettop(L) == 0)
    {
        // io.lines() case, just call realio.lines
        lua_getfield(L, LUA_REGISTRYINDEX, REG_REAL_IO_NAME);
        lua_getfield(L, -1, "lines");
        lua_remove(L, -2);  // remove LUA_REGISTRYINDEX[REAL_IO_NAME]
        lua_call(L, 0, 1);
        return 1;
    }
    QString filename;
    if (!lua::pop(L, &filename))
    {
        return luaL_error(
            L,
            "io.lines filename (1st argument) must be a string or not present");
    }
    QFileInfo file(pl->dataDirectory().filePath(filename));
    auto abs = file.absoluteFilePath();
    qCDebug(chatterinoLua) << "[" << pl->id << ":" << pl->meta.name
                           << "] Plugin is opening file at " << abs
                           << " for reading lines";
    bool ok = pl->hasFSPermissionFor(false, abs);
    if (!ok)
    {
        return ioError(L,
                       "Plugin does not have permissions to access given file.",
                       EACCES);
    }
    // Our stack looks like this:
    // - {...}[1]
    // - {...}[2]
    // ...
    // We want:
    // - REG[REG_REAL_IO_NAME].lines
    // - absolute file path
    // - {...}[1]
    // - {...}[2]
    // ...

    lua_getfield(L, LUA_REGISTRYINDEX, REG_REAL_IO_NAME);
    lua_getfield(L, -1, "lines");
    lua_remove(L, -2);  // remove LUA_REGISTRYINDEX[REAL_IO_NAME]
    lua_insert(L, 1);   // move function to start of stack
    lua::push(L, abs);
    lua_insert(L, 2);  // move file name just after the function
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

namespace {

    // This is the code for both io.input and io.output
    int globalFileCommon(lua_State *L, bool output)
    {
        auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
        if (pl == nullptr)
        {
            luaL_error(L, "internal error: no plugin");
            return 0;
        }
        // Three signature cases:
        // io.input()
        // io.input(file)
        // io.input(name)
        if (lua_gettop(L) == 0)
        {
            // We have no arguments, call realio.input()
            lua_getfield(L, LUA_REGISTRYINDEX, REG_REAL_IO_NAME);
            if (output)
            {
                lua_getfield(L, -1, "output");
            }
            else
            {
                lua_getfield(L, -1, "input");
            }
            lua_remove(L, -2);  // remove LUA_REGISTRYINDEX[REAL_IO_NAME]
            lua_call(L, 0, 1);
            return 1;
        }
        if (lua_gettop(L) != 1)
        {
            return luaL_error(L, "Too many arguments given to io.input().");
        }
        // Now check if we have a file or name
        auto *p = luaL_testudata(L, 1, LUA_FILEHANDLE);
        if (p == nullptr)
        {
            // this is not a file handle, send it to open
            luaL_getsubtable(L, LUA_REGISTRYINDEX, REG_C2_IO_NAME);
            lua_getfield(L, -1, "open");
            lua_remove(L, -2);  // remove io

            lua_pushvalue(L, 1);  // dupe arg
            if (output)
            {
                lua_pushstring(L, "w");
            }
            else
            {
                lua_pushstring(L, "r");
            }
            lua_call(L, 2, 1);  // call ourio.open(arg1, 'r'|'w')
            // if this isn't a string ourio.open errors

            // this leaves us with:
            // 1. arg
            // 2. new_file
            lua_remove(L, 1);  // remove arg, replacing it with new_file
        }

        // file handle, pass it off to realio.input
        lua_getfield(L, LUA_REGISTRYINDEX, REG_REAL_IO_NAME);
        if (output)
        {
            lua_getfield(L, -1, "output");
        }
        else
        {
            lua_getfield(L, -1, "input");
        }
        lua_remove(L, -2);    // remove LUA_REGISTRYINDEX[REAL_IO_NAME]
        lua_pushvalue(L, 1);  // duplicate arg
        lua_call(L, 1, 1);
        return 1;
    }

}  // namespace

int io_input(lua_State *L)
{
    return globalFileCommon(L, false);
}

int io_output(lua_State *L)
{
    return globalFileCommon(L, true);
}

int io_close(lua_State *L)
{
    if (lua_gettop(L) > 1)
    {
        return luaL_error(
            L, "Too many arguments for io.close. Expected one or zero.");
    }
    if (lua_gettop(L) == 0)
    {
        lua_getfield(L, LUA_REGISTRYINDEX, "_IO_output");
    }
    lua_getfield(L, -1, "close");
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}

int io_flush(lua_State *L)
{
    if (lua_gettop(L) > 1)
    {
        return luaL_error(
            L, "Too many arguments for io.flush. Expected one or zero.");
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "_IO_output");
    lua_getfield(L, -1, "flush");
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}

int io_read(lua_State *L)
{
    if (lua_gettop(L) > 1)
    {
        return luaL_error(
            L, "Too many arguments for io.read. Expected one or zero.");
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "_IO_input");
    lua_getfield(L, -1, "read");
    lua_insert(L, 1);
    lua_insert(L, 2);
    lua_call(L, lua_gettop(L) - 1, 1);
    return 1;
}

int io_write(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_IO_output");
    lua_getfield(L, -1, "write");
    lua_insert(L, 1);
    lua_insert(L, 2);
    // (input)
    // (input).read
    // args
    lua_call(L, lua_gettop(L) - 1, 1);
    return 1;
}

int io_popen(lua_State *L)
{
    return luaL_error(L, "io.popen: This function is a stub!");
}

int io_tmpfile(lua_State *L)
{
    return luaL_error(L, "io.tmpfile: This function is a stub!");
}

// NOLINTEND(*vararg)

}  // namespace chatterino::lua::api
#endif
