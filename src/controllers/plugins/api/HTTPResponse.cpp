#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPResponse.hpp"

#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "util/DebugCount.hpp"

#    include <lauxlib.h>

#    include <utility>

namespace chatterino::lua::api {
// NOLINTBEGIN(*vararg)
// NOLINTNEXTLINE(*-avoid-c-arrays)
static const luaL_Reg HTTP_RESPONSE_METHODS[] = {
    {"data", &HTTPResponse::data_wrap},
    {"status", &HTTPResponse::status_wrap},
    {"error", &HTTPResponse::error_wrap},
    {nullptr, nullptr},
};

void HTTPResponse::createMetatable(lua_State *L)
{
    lua::StackGuard guard(L, 1);

    luaL_newmetatable(L, "c2.HTTPResponse");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  // clone metatable
    lua_settable(L, -3);   // metatable.__index = metatable

    // Generic ISharedResource stuff
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, (&SharedPtrUserData<UserData::Type::HTTPResponse,
                                             HTTPResponse>::destroy));
    lua_settable(L, -3);  // metatable.__gc = SharedPtrUserData<...>::destroy

    luaL_setfuncs(L, HTTP_RESPONSE_METHODS, 0);
}

std::shared_ptr<HTTPResponse> HTTPResponse::getOrError(lua_State *L,
                                                       StackIdx where)
{
    if (lua_gettop(L) < 1)
    {
        // The nullptr is there just to appease the compiler, luaL_error is no return
        luaL_error(L, "Called c2.HTTPResponse method without a request object");
        return nullptr;
    }
    if (lua_isuserdata(L, where) == 0)
    {
        luaL_error(L, "Called c2.HTTPResponse method with a non-userdata "
                      "'self' argument");
        return nullptr;
    }
    // luaL_checkudata is no-return if check fails
    auto *checked = luaL_checkudata(L, where, "c2.HTTPResponse");
    auto *data =
        SharedPtrUserData<UserData::Type::HTTPResponse, HTTPResponse>::from(
            checked);
    if (data == nullptr)
    {
        luaL_error(L, "Called c2.HTTPResponse method with an invalid pointer");
        return nullptr;
    }
    lua_remove(L, where);
    if (data->target == nullptr)
    {
        luaL_error(
            L,
            "Internal error: SharedPtrUserData<UserData::Type::HTTPResponse, "
            "HTTPResponse>::target was null. This is a Chatterino bug!");
        return nullptr;
    }
    return data->target;
}

HTTPResponse::HTTPResponse(NetworkResult res)
    : result_(std::move(res))
{
    DebugCount::increase("lua::api::HTTPResponse");
}
HTTPResponse::~HTTPResponse()
{
    DebugCount::decrease("lua::api::HTTPResponse");
}

int HTTPResponse::data_wrap(lua_State *L)
{
    lua::StackGuard guard(L, 0);  // 1 in, 1 out
    auto ptr = HTTPResponse::getOrError(L, 1);
    return ptr->data(L);
}

int HTTPResponse::data(lua_State *L)
{
    lua::push(L, this->result_.getData().toStdString());
    return 1;
}

int HTTPResponse::status_wrap(lua_State *L)
{
    lua::StackGuard guard(L, 0);  // 1 in, 1 out
    auto ptr = HTTPResponse::getOrError(L, 1);
    return ptr->status(L);
}

int HTTPResponse::status(lua_State *L)
{
    lua::push(L, this->result_.status());
    return 1;
}

int HTTPResponse::error_wrap(lua_State *L)
{
    lua::StackGuard guard(L, 0);  // 1 in, 1 out
    auto ptr = HTTPResponse::getOrError(L, 1);
    return ptr->error(L);
}

int HTTPResponse::error(lua_State *L)
{
    lua::push(L, this->result_.formatError());
    return 1;
}

// NOLINTEND(*vararg)
}  // namespace chatterino::lua::api

namespace chatterino::lua {
StackIdx push(lua_State *L, std::shared_ptr<api::HTTPResponse> request)
{
    using namespace chatterino::lua::api;

    // Prepare table
    SharedPtrUserData<UserData::Type::HTTPResponse, HTTPResponse>::create(
        L, std::move(request));
    luaL_getmetatable(L, "c2.HTTPResponse");
    lua_setmetatable(L, -2);

    return lua_gettop(L);
}
}  // namespace chatterino::lua
#endif
