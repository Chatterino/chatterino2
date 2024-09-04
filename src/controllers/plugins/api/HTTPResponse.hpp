#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"

#    include <lua.h>

#    include <memory>

namespace chatterino {
class PluginController;
}  // namespace chatterino

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @lua@class HTTPResponse
 */
class HTTPResponse : public std::enable_shared_from_this<HTTPResponse>
{
    NetworkResult result_;

public:
    HTTPResponse(NetworkResult res);
    HTTPResponse(HTTPResponse &&other) = default;
    HTTPResponse &operator=(HTTPResponse &&) = default;
    HTTPResponse &operator=(HTTPResponse &) = delete;
    HTTPResponse(const HTTPResponse &other) = delete;
    ~HTTPResponse();

private:
    static void createMetatable(lua_State *L);
    friend class chatterino::PluginController;

    /**
     * @brief Get the content of the top object on Lua stack, usually the first argument as an HTTPResponse
     *
     * If the object given is not a userdatum or the pointer inside that
     * userdatum doesn't point to a HTTPResponse, a lua error is thrown.
     *
     * This function always returns a non-null pointer.
     */
    static std::shared_ptr<HTTPResponse> getOrError(lua_State *L,
                                                    StackIdx where = -1);

public:
    /**
     * Returns the data. This is not guaranteed to be encoded using any
     * particular encoding scheme. It's just the bytes the server returned.
     * 
     * @exposed HTTPResponse:data
     */
    static int data_wrap(lua_State *L);
    int data(lua_State *L);

    /**
     * Returns the status code.
     *
     * @exposed HTTPResponse:status
     */
    static int status_wrap(lua_State *L);
    int status(lua_State *L);

    /**
     * A somewhat human readable description of an error if such happened
     * @exposed HTTPResponse:error
     */

    static int error_wrap(lua_State *L);
    int error(lua_State *L);
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
namespace chatterino::lua {
StackIdx push(lua_State *L, std::shared_ptr<api::HTTPResponse> request);
}  // namespace chatterino::lua
#endif
