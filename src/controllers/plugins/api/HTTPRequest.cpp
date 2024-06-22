#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPRequest.hpp"

#    include "Application.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/api/HTTPResponse.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "util/DebugCount.hpp"

extern "C" {
#    include <lauxlib.h>
#    include <lua.h>
}
#    include <QRandomGenerator>
#    include <QUrl>

#    include <memory>
#    include <utility>

namespace chatterino::lua::api {
// NOLINTBEGIN(*vararg)
// NOLINTNEXTLINE(*-avoid-c-arrays)
static const luaL_Reg HTTP_REQUEST_METHODS[] = {
    {"on_success", &HTTPRequest::on_success_wrap},
    {"on_error", &HTTPRequest::on_error_wrap},
    {"finally", &HTTPRequest::finally_wrap},

    {"execute", &HTTPRequest::execute_wrap},
    {"set_timeout", &HTTPRequest::set_timeout_wrap},
    {"set_payload", &HTTPRequest::set_payload_wrap},
    {"set_header", &HTTPRequest::set_header_wrap},
    // static
    {"create", &HTTPRequest::create},
    {nullptr, nullptr},
};

std::shared_ptr<HTTPRequest> HTTPRequest::getOrError(lua_State *L,
                                                     StackIdx where)
{
    if (lua_gettop(L) < 1)
    {
        // The nullptr is there just to appease the compiler, luaL_error is no return
        luaL_error(L, "Called c2.HTTPRequest method without a request object");
        return nullptr;
    }
    if (lua_isuserdata(L, where) == 0)
    {
        luaL_error(
            L,
            "Called c2.HTTPRequest method with a non-userdata 'self' argument");
        return nullptr;
    }
    // luaL_checkudata is no-return if check fails
    auto *checked = luaL_checkudata(L, where, "c2.HTTPRequest");
    auto *data =
        SharedPtrUserData<UserData::Type::HTTPRequest, HTTPRequest>::from(
            checked);
    if (data == nullptr)
    {
        luaL_error(L, "Called c2.HTTPRequest method with an invalid pointer");
        return nullptr;
    }
    lua_remove(L, where);
    if (data->target == nullptr)
    {
        luaL_error(
            L, "Internal error: SharedPtrUserData<UserData::Type::HTTPRequest, "
               "HTTPRequest>::target was null. This is a Chatterino bug!");
        return nullptr;
    }
    if (data->target->done)
    {
        luaL_error(L, "This c2.HTTPRequest has already been executed!");
        return nullptr;
    }
    return data->target;
}

void HTTPRequest::createMetatable(lua_State *L)
{
    lua::StackGuard guard(L, 1);

    luaL_newmetatable(L, "c2.HTTPRequest");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  // clone metatable
    lua_settable(L, -3);   // metatable.__index = metatable

    // Generic ISharedResource stuff
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, (&SharedPtrUserData<UserData::Type::HTTPRequest,
                                             HTTPRequest>::destroy));
    lua_settable(L, -3);  // metatable.__gc = SharedPtrUserData<...>::destroy

    luaL_setfuncs(L, HTTP_REQUEST_METHODS, 0);
}

int HTTPRequest::on_success_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -2);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->on_success(L);
}

int HTTPRequest::on_success(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        return luaL_error(
            L, "HTTPRequest:on_success needs 1 argument (a callback "
               "that takes an HTTPResult and doesn't return anything)");
    }
    if (!lua_isfunction(L, top))
    {
        return luaL_error(
            L, "HTTPRequest:on_success needs 1 argument (a callback "
               "that takes an HTTPResult and doesn't return anything)");
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "success");  // this deletes the function copy
    lua_pop(L, 2);  // delete the table and function original
    return 0;
}

int HTTPRequest::on_error_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -2);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->on_error(L);
}

int HTTPRequest::on_error(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        return luaL_error(
            L, "HTTPRequest:on_error needs 1 argument (a callback "
               "that takes an HTTPResult and doesn't return anything)");
    }
    if (!lua_isfunction(L, top))
    {
        return luaL_error(
            L, "HTTPRequest:on_error needs 1 argument (a callback "
               "that takes an HTTPResult and doesn't return anything)");
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "error");  // this deletes the function copy
    lua_pop(L, 2);                     // delete the table and function original
    return 0;
}

int HTTPRequest::set_timeout_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -2);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->set_timeout(L);
}

int HTTPRequest::set_timeout(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        return luaL_error(
            L, "HTTPRequest:set_timeout needs 1 argument (a number of "
               "milliseconds after which the request will time out)");
    }

    int temporary = -1;
    if (!lua::pop(L, &temporary))
    {
        return luaL_error(
            L, "HTTPRequest:set_timeout failed to get timeout, expected a "
               "positive integer");
    }
    if (temporary <= 0)
    {
        return luaL_error(
            L, "HTTPRequest:set_timeout failed to get timeout, expected a "
               "positive integer");
    }
    this->timeout_ = temporary;
    return 0;
}

int HTTPRequest::finally_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -2);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->finally(L);
}

int HTTPRequest::finally(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        return luaL_error(L, "HTTPRequest:finally needs 1 argument (a callback "
                             "that takes nothing and doesn't return anything)");
    }
    if (!lua_isfunction(L, top))
    {
        return luaL_error(L, "HTTPRequest:finally needs 1 argument (a callback "
                             "that takes nothing and doesn't return anything)");
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "finally");  // this deletes the function copy
    lua_pop(L, 2);  // delete the table and function original
    return 0;
}

int HTTPRequest::set_payload_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -2);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->set_payload(L);
}

int HTTPRequest::set_payload(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        return luaL_error(
            L, "HTTPRequest:set_payload needs 1 argument (a string payload)");
    }

    std::string temporary;
    if (!lua::pop(L, &temporary))
    {
        return luaL_error(
            L, "HTTPRequest:set_payload failed to get payload, expected a "
               "string");
    }
    this->req_ =
        std::move(this->req_).payload(QByteArray::fromStdString(temporary));
    return 0;
}

int HTTPRequest::set_header_wrap(lua_State *L)
{
    lua::StackGuard guard(L, -3);
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->set_header(L);
}

int HTTPRequest::set_header(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 2)
    {
        return luaL_error(
            L, "HTTPRequest:set_header needs 2 arguments (a header name "
               "and a value)");
    }

    std::string value;
    if (!lua::pop(L, &value))
    {
        return luaL_error(
            L, "cannot get value (2nd argument of HTTPRequest:set_header)");
    }
    std::string name;
    if (!lua::pop(L, &name))
    {
        return luaL_error(
            L, "cannot get name (1st argument of HTTPRequest:set_header)");
    }
    this->req_ = std::move(this->req_)
                     .header(QByteArray::fromStdString(name),
                             QByteArray::fromStdString(value));
    return 0;
}

int HTTPRequest::create(lua_State *L)
{
    lua::StackGuard guard(L, -1);
    if (lua_gettop(L) != 2)
    {
        return luaL_error(
            L, "HTTPRequest.create needs exactly 2 arguments (method "
               "and url)");
    }
    QString url;
    if (!lua::pop(L, &url))
    {
        return luaL_error(L,
                          "cannot get url (2nd argument of HTTPRequest.create, "
                          "expected a string)");
    }
    auto parsedurl = QUrl(url);
    if (!parsedurl.isValid())
    {
        return luaL_error(
            L, "cannot parse url (2nd argument of HTTPRequest.create, "
               "got invalid url in argument)");
    }
    NetworkRequestType method{};
    if (!lua::pop(L, &method))
    {
        return luaL_error(
            L, "cannot get method (1st argument of HTTPRequest.create, "
               "expected a string)");
    }
    auto *pl = getIApp()->getPlugins()->getPluginByStatePtr(L);
    if (!pl->hasHTTPPermissionFor(parsedurl))
    {
        return luaL_error(
            L, "Plugin does not have permission to send HTTP requests "
               "to this URL");
    }
    NetworkRequest r(parsedurl, method);
    lua::push(
        L, std::make_shared<HTTPRequest>(ConstructorAccessTag{}, std::move(r)));
    return 1;
}

int HTTPRequest::execute_wrap(lua_State *L)
{
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->execute(L);
}

int HTTPRequest::execute(lua_State *L)
{
    auto shared = this->shared_from_this();
    this->done = true;
    std::move(this->req_)
        .onSuccess([shared, L](const NetworkResult &res) {
            lua::StackGuard guard(L);
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "success");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                lua::push(thread, std::make_shared<HTTPResponse>(res));
                // one arg, no return, no msgh
                lua_pcall(thread, 1, 0, 0);
            }
            else
            {
                lua_pop(thread, 1);  // remove callback
            }
            lua_closethread(thread, nullptr);
            lua_pop(L, 1);  // remove thread from L
        })
        .onError([shared, L](const NetworkResult &res) {
            lua::StackGuard guard(L);
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "error");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                lua::push(thread, std::make_shared<HTTPResponse>(res));
                // one arg, no return, no msgh
                lua_pcall(thread, 1, 0, 0);
            }
            else
            {
                lua_pop(thread, 1);  // remove callback
            }
            lua_closethread(thread, nullptr);
            lua_pop(L, 1);  // remove thread from L
        })
        .finally([shared, L]() {
            lua::StackGuard guard(L);
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "finally");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                // no args, no return, no msgh
                lua_pcall(thread, 0, 0, 0);
            }
            else
            {
                lua_pop(thread, 1);  // remove callback
            }
            // remove our private data
            lua_pushnil(thread);
            lua_setfield(thread, LUA_REGISTRYINDEX,
                         shared->privateKey.toStdString().c_str());
            lua_closethread(thread, nullptr);
            lua_pop(L, 1);  // remove thread from L

            // we removed our private table, forget the key for it
            shared->privateKey = QString();
        })
        .timeout(this->timeout_)
        .execute();
    return 0;
}

HTTPRequest::HTTPRequest(HTTPRequest::ConstructorAccessTag /*ignored*/,
                         NetworkRequest req)
    : req_(std::move(req))
{
    DebugCount::increase("lua::api::HTTPRequest");
}

HTTPRequest::~HTTPRequest()
{
    DebugCount::decrease("lua::api::HTTPRequest");
    // We might leak a Lua function or two here if the request isn't executed
    // but that's better than accessing a possibly invalid lua_State pointer.
}

StackIdx HTTPRequest::pushPrivate(lua_State *L)
{
    if (this->privateKey.isEmpty())
    {
        this->privateKey = QString("HTTPRequestPrivate%1")
                               .arg(QRandomGenerator::system()->generate());
        pushEmptyTable(L, 4);
        lua_setfield(L, LUA_REGISTRYINDEX,
                     this->privateKey.toStdString().c_str());
    }
    lua_getfield(L, LUA_REGISTRYINDEX, this->privateKey.toStdString().c_str());
    return lua_gettop(L);
}

// NOLINTEND(*vararg)
}  // namespace chatterino::lua::api

namespace chatterino::lua {

StackIdx push(lua_State *L, std::shared_ptr<api::HTTPRequest> request)
{
    using namespace chatterino::lua::api;

    SharedPtrUserData<UserData::Type::HTTPRequest, HTTPRequest>::create(
        L, std::move(request));
    luaL_getmetatable(L, "c2.HTTPRequest");
    lua_setmetatable(L, -2);
    return lua_gettop(L);
}
}  // namespace chatterino::lua
#endif
