#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/IOWrapper.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/plugins/PluginController.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <QString>
#    include <sol/sol.hpp>

#    include <cerrno>
#    include <stdexcept>
#    include <utility>

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

sol::variadic_results ioError(lua_State *L, const QString &value,
                              int errnoequiv)
{
    sol::variadic_results out;
    out.push_back(sol::nil);
    out.push_back(sol::make_object(L, value.toStdString()));
    out.push_back({L, sol::in_place_type<int>, errnoequiv});
    return out;
}

sol::variadic_results io_open(sol::this_state L, QString filename,
                              QString strmode)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    LuaFileMode mode(strmode);
    if (!mode.error.isEmpty())
    {
        throw std::runtime_error(mode.error.toStdString());
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

    sol::state_view lua(L);
    auto open = lua.registry()[REG_REAL_IO_NAME]["open"];
    sol::protected_function_result res =
        open(abs.toStdString(), mode.toString().toStdString());
    return res;
}
sol::variadic_results io_open_modeless(sol::this_state L, QString filename)
{
    return io_open(L, std::move(filename), "r");
}

sol::variadic_results io_lines_noargs(sol::this_state L)
{
    sol::state_view lua(L);
    auto lines = lua.registry()[REG_REAL_IO_NAME]["lines"];
    sol::protected_function_result res = lines();
    return res;
}

sol::variadic_results io_lines(sol::this_state L, QString filename,
                               sol::variadic_args args)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);
    QFileInfo file(pl->dataDirectory().filePath(filename));
    auto abs = file.absoluteFilePath();
    qCDebug(chatterinoLua) << "[" << pl->id << ":" << pl->meta.name
                           << "] Plugin is opening file at " << abs
                           << " for reading lines";
    bool ok = pl->hasFSPermissionFor(false, abs);
    if (!ok)
    {
        throw std::runtime_error(
            "Plugin does not have permissions to access given file.");
    }

    auto lines = lua.registry()[REG_REAL_IO_NAME]["lines"];
    sol::protected_function_result res = lines(abs.toStdString(), args);
    return res;
}

sol::variadic_results io_input_argless(sol::this_state L)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);

    auto func = lua.registry()[REG_REAL_IO_NAME]["input"];
    sol::protected_function_result res = func();
    return res;
}
sol::variadic_results io_input_file(sol::this_state L, sol::userdata file)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);

    auto func = lua.registry()[REG_REAL_IO_NAME]["input"];
    sol::protected_function_result res = func(file);
    return res;
}
sol::variadic_results io_input_name(sol::this_state L, QString filename)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);
    auto res = io_open(L, std::move(filename), "r");
    if (res.size() != 1)
    {
        throw std::runtime_error(res.at(1).as<std::string>());
    }
    auto obj = res.at(0);
    if (obj.get_type() != sol::type::userdata)
    {
        throw std::runtime_error("a file must be a userdata.");
    }
    return io_input_file(L, obj);
}

sol::variadic_results io_output_argless(sol::this_state L)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);

    auto func = lua.registry()[REG_REAL_IO_NAME]["output"];
    sol::protected_function_result res = func();
    return res;
}
sol::variadic_results io_output_file(sol::this_state L, sol::userdata file)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);

    auto func = lua.registry()[REG_REAL_IO_NAME]["output"];
    sol::protected_function_result res = func(file);
    return res;
}
sol::variadic_results io_output_name(sol::this_state L, QString filename)
{
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (pl == nullptr)
    {
        throw std::runtime_error("internal error: no plugin");
    }
    sol::state_view lua(L);
    auto res = io_open(L, std::move(filename), "w");
    if (res.size() != 1)
    {
        throw std::runtime_error(res.at(1).as<std::string>());
    }
    auto obj = res.at(0);
    if (obj.get_type() != sol::type::userdata)
    {
        throw std::runtime_error("internal error: a file must be a userdata.");
    }
    return io_output_file(L, obj);
}

bool io_close_argless(sol::this_state L)
{
    sol::state_view lua(L);
    auto out = lua.registry()["_IO_output"];
    return io_close_file(L, out);
}

bool io_close_file(sol::this_state L, sol::userdata file)
{
    sol::state_view lua(L);
    return file["close"](file);
}

void io_flush_argless(sol::this_state L)
{
    sol::state_view lua(L);
    auto out = lua.registry()["_IO_output"];
    io_flush_file(L, out);
}

void io_flush_file(sol::this_state L, sol::userdata file)
{
    sol::state_view lua(L);
    file["flush"](file);
}

sol::variadic_results io_read(sol::this_state L, sol::variadic_args args)
{
    sol::state_view lua(L);
    auto inp = lua.registry()["_IO_input"];
    if (!inp.is<sol::userdata>())
    {
        throw std::runtime_error("Input not set to a file");
    }
    sol::protected_function read = inp["read"];
    return read(inp, args);
}

sol::variadic_results io_write(sol::this_state L, sol::variadic_args args)
{
    sol::state_view lua(L);
    auto out = lua.registry()["_IO_output"];
    if (!out.is<sol::userdata>())
    {
        throw std::runtime_error("Output not set to a file");
    }
    sol::protected_function write = out["write"];
    return write(out, args);
}

void io_popen()
{
    throw std::runtime_error("io.popen: This function is a stub!");
}

void io_tmpfile()
{
    throw std::runtime_error("io.tmpfile: This function is a stub!");
}

}  // namespace chatterino::lua::api
#endif
