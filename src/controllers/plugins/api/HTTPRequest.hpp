#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkRequest.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"

#    include <memory>

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @lua@alias HTTPCallback fun(result: HTTPResponse): nil
 */

/**
 * @lua@class HTTPRequest
 */
class HTTPRequest : public std::enable_shared_from_this<HTTPRequest>
{
    // This type is private to prevent the accidental construction of HTTPRequest without a shared pointer
    struct ConstructorAccessTag {
    };

public:
    HTTPRequest(HTTPRequest::ConstructorAccessTag, NetworkRequest req);
    HTTPRequest(HTTPRequest &&other) = default;
    HTTPRequest &operator=(HTTPRequest &&) = default;
    HTTPRequest &operator=(HTTPRequest &) = delete;
    HTTPRequest(const HTTPRequest &other) = delete;
    ~HTTPRequest();

private:
    NetworkRequest req_;

    static void createMetatable(lua_State *L);
    friend class chatterino::PluginController;

    /**
     * @brief Get the content of the top object on Lua stack, usually the first argument as an HTTPRequest
     *
     * If the object given is not a userdatum or the pointer inside that
     * userdatum doesn't point to a HTTPRequest, a lua error is thrown.
     *
     * This function always returns a non-null pointer.
     */
    static std::shared_ptr<HTTPRequest> getOrError(lua_State *L,
                                                   StackIdx where = -1);
    /**
     * Pushes the private table onto the lua stack.
     *
     * This might create it if it doesn't exist.
     */
    StackIdx pushPrivate(lua_State *L);

    // This is the key in the registry the private table it held at (if it exists)
    // This might be a null QString if the request has already been executed or
    // the table wasn't created yet.
    QString privateKey;
    int timeout_ = 10'000;
    bool done = false;

public:
    // These functions are wrapped so data can be accessed more easily. When a call from Lua comes in:
    //  - the static wrapper function is called
    //  - it calls getOrError
    //  - and then the wrapped method

    /**
     * Sets the success callback
     *
     * @lua@param callback HTTPCallback Function to call when the HTTP request succeeds
     * @exposed HTTPRequest:on_success
     */
    static int on_success_wrap(lua_State *L);
    int on_success(lua_State *L);

    /**
     * Sets the failure callback
     *
     * @lua@param callback HTTPCallback Function to call when the HTTP request fails or returns a non-ok status
     * @exposed HTTPRequest:on_error
     */
    static int on_error_wrap(lua_State *L);
    int on_error(lua_State *L);

    /**
     * Sets the finally callback
     *
     * @lua@param callback fun(): nil Function to call when the HTTP request finishes
     * @exposed HTTPRequest:finally
     */
    static int finally_wrap(lua_State *L);
    int finally(lua_State *L);

    /**
     * Sets the timeout
     *
     * @lua@param timeout integer How long in milliseconds until the times out
     * @exposed HTTPRequest:set_timeout
     */
    static int set_timeout_wrap(lua_State *L);
    int set_timeout(lua_State *L);

    /**
     * Sets the request payload
     *
     * @lua@param data string
     * @exposed HTTPRequest:set_payload
     */
    static int set_payload_wrap(lua_State *L);
    int set_payload(lua_State *L);

    /**
     * Sets a header in the request
     *
     * @lua@param name string
     * @lua@param value string
     * @exposed HTTPRequest:set_header
     */
    static int set_header_wrap(lua_State *L);
    int set_header(lua_State *L);

    /**
     * Executes the HTTP request
     *
     * @exposed HTTPRequest:execute
     */
    static int execute_wrap(lua_State *L);
    int execute(lua_State *L);

    /**
     * Static functions
     */

    /**
     * Creates a new HTTPRequest
     *
     * @lua@param method HTTPMethod Method to use
     * @lua@param url string Where to send the request to
     *
     * @lua@return HTTPRequest
     * @exposed HTTPRequest.create
     */
    static int create(lua_State *L);
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api

namespace chatterino::lua {
StackIdx push(lua_State *L, std::shared_ptr<api::HTTPRequest> request);
}  // namespace chatterino::lua

#endif
