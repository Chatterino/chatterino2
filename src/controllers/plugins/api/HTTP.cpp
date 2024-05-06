#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTP.hpp"

#    include "common/network/NetworkCommon.hpp"
#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"

extern "C" {
#    include <lauxlib.h>
#    include <lua.h>
}
#    include <QRandomGenerator>
#    include <QUrl>

#    include <utility>

namespace chatterino::lua::api {
// NOLINTBEGIN(*vararg)
// NOLINTNEXTLINE(*-avoid-c-arrays)
static const luaL_Reg HTTP_REQUEST_METHODS[] = {
    {"on_success", &HTTPRequest::on_success_wrap},
    {"on_error", &HTTPRequest::on_error_wrap},
    {"finally", &HTTPRequest::finally_wrap},

    {"execute", &HTTPRequest::execute_wrap},
    // static
    {"create", &HTTPRequest::create},
    {nullptr, nullptr},
};

std::shared_ptr<HTTPRequest> HTTPRequest::getOrError(lua_State *L,
                                                     StackIdx where)
{
    if (lua_gettop(L) < 1)
    {
        luaL_error(L, "Called c2.HTTPRequest method without a channel object");
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
    if (data->target->done)
    {
        luaL_error(L, "This c2.HTTPRequest has already been executed!");
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
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->on_success(L);
}

int HTTPRequest::on_success(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        luaL_error(L, "HTTPRequest:on_success needs 1 argument (a callback "
                      "that takes an HTTPResult and doesn't return anything)");
        return 0;
    }
    if (!lua_isfunction(L, top))
    {
        luaL_error(L, "HTTPRequest:on_success needs 1 argument (a callback "
                      "that takes an HTTPResult and doesn't return anything)");
        return 0;
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "success");
    return 0;
}

int HTTPRequest::on_error_wrap(lua_State *L)
{
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->on_error(L);
}

int HTTPRequest::on_error(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        luaL_error(L, "HTTPRequest:on_error needs 1 argument (a callback "
                      "that takes an HTTPResult and doesn't return anything)");
        return 0;
    }
    if (!lua_isfunction(L, top))
    {
        luaL_error(L, "HTTPRequest:on_error needs 1 argument (a callback "
                      "that takes an HTTPResult and doesn't return anything)");
        return 0;
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "error");
    return 0;
}

int HTTPRequest::finally_wrap(lua_State *L)
{
    auto ptr = HTTPRequest::getOrError(L, 1);
    return ptr->finally(L);
}

int HTTPRequest::finally(lua_State *L)
{
    auto top = lua_gettop(L);
    if (top != 1)
    {
        luaL_error(L, "HTTPRequest:finally needs 1 argument (a callback "
                      "that takes nothing and doesn't return anything)");
        return 0;
    }
    if (!lua_isfunction(L, top))
    {
        luaL_error(L, "HTTPRequest:finally needs 1 argument (a callback "
                      "that takes nothing and doesn't return anything)");
        return 0;
    }
    auto shared = this->pushPrivate(L);
    lua_pushvalue(L, -2);
    lua_setfield(L, shared, "finally");
    return 0;
}

int HTTPRequest::create(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        luaL_error(L, "HTTPRequest.create needs exactly 2 arguments (method "
                      "and url)");
        lua_pushnil(L);
        return 1;
    }
    QString url;
    if (!lua::pop(L, &url))
    {
        luaL_error(L, "cannot get url (2nd argument of HTTPRequest.create, "
                      "expected a string)");
        lua_pushnil(L);
        return 1;
    }
    auto parsedurl = QUrl(url);
    if (!parsedurl.isValid())
    {
        luaL_error(L, "cannot parse url (2nd argument of HTTPRequest.create, "
                      "got invalid url in argument)");
        lua_pushnil(L);
        return 1;
    }
    NetworkRequestType method{};
    if (!lua::pop(L, &method))
    {
        luaL_error(L, "cannot get method (1st argument of HTTPRequest.create, "
                      "expected a string)");
        lua_pushnil(L);
        return 1;
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
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "success");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                lua::push(thread, res.getData().toStdString());
                // one arg, no return, no msgh
                lua_pcall(thread, 1, 0, 0);
            }
            lua_closethread(thread, nullptr);
        })
        .onError([shared, L](const NetworkResult &res) {
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "error");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                lua::push(thread, res.getData().toStdString());
                // one arg, no return, no msgh
                lua_pcall(thread, 1, 0, 0);
            }
            lua_closethread(thread, nullptr);
        })
        .finally([shared, L]() {
            auto *thread = lua_newthread(L);

            auto priv = shared->pushPrivate(thread);
            lua_getfield(thread, priv, "finally");
            auto cb = lua_gettop(thread);
            if (lua_isfunction(thread, cb))
            {
                // no args, no return, no msgh
                lua_pcall(thread, 0, 0, 0);
            }
            lua_closethread(thread, nullptr);
        })
        .timeout(this->timeout_)
        .execute();
    return 0;
}

HTTPRequest::HTTPRequest(HTTPRequest::ConstructorAccessTag /*ignored*/,
                         NetworkRequest req)
    : req_(std::move(req))
{
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
