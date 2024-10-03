#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkRequest.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"

#    include <sol/forward.hpp>

#    include <memory>

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @lua@alias HTTPCallback fun(result: HTTPResponse): nil
 */

/**
 * @lua@class HTTPRequest
 */
class HTTPRequest
{
    // This type is private to prevent the accidental construction of HTTPRequest without a shared pointer
    struct ConstructorAccessTag {
    };

public:
    HTTPRequest(HTTPRequest::ConstructorAccessTag, NetworkRequest req,
                lua_State *state);
    HTTPRequest(HTTPRequest &&other) = default;
    HTTPRequest &operator=(HTTPRequest &&) = default;
    HTTPRequest &operator=(HTTPRequest &) = delete;
    HTTPRequest(const HTTPRequest &other) = delete;
    ~HTTPRequest();

private:
    NetworkRequest req_;

    static void createUserType(lua_State *L, sol::table &c2);
    friend class chatterino::PluginController;

    // This is the key in the registry the private table it held at (if it exists)
    // This might be a null QString if the request has already been executed or
    // the table wasn't created yet.
    int timeout_ = 10'000;
    bool done = false;
    lua_State *state_;

    std::optional<sol::protected_function> cbSuccess;
    std::optional<sol::protected_function> cbError;
    std::optional<sol::protected_function> cbFinally;

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
    void on_success(sol::protected_function func);

    /**
     * Sets the failure callback
     *
     * @lua@param callback HTTPCallback Function to call when the HTTP request fails or returns a non-ok status
     * @exposed HTTPRequest:on_error
     */
    void on_error(sol::protected_function func);

    /**
     * Sets the finally callback
     *
     * @lua@param callback fun(): nil Function to call when the HTTP request finishes
     * @exposed HTTPRequest:finally
     */
    void finally(sol::protected_function func);

    /**
     * Sets the timeout
     *
     * @lua@param timeout integer How long in milliseconds until the times out
     * @exposed HTTPRequest:set_timeout
     */
    void set_timeout(int timeout);

    /**
     * Sets the request payload
     *
     * @lua@param data string
     * @exposed HTTPRequest:set_payload
     */
    void set_payload(const std::string &payload);

    /**
     * Sets a header in the request
     *
     * @lua@param name string
     * @lua@param value string
     * @exposed HTTPRequest:set_header
     */
    void set_header(std::string name, std::string value);

    /**
     * Executes the HTTP request
     *
     * @exposed HTTPRequest:execute
     */
    void execute();

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
    static HTTPRequest create(lua_State *L, NetworkRequestType method,
                              QString url);
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api

namespace chatterino::lua {
StackIdx push(lua_State *L, std::shared_ptr<api::HTTPRequest> request);
}  // namespace chatterino::lua

#endif
