#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkRequest.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"

#    include <sol/forward.hpp>
#    include <sol/types.hpp>

#    include <memory>

namespace chatterino {
class PluginController;
}  // namespace chatterino

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @lua@alias c2.HTTPCallback fun(result: c2.HTTPResponse): nil
 */

/**
 * @lua@class c2.HTTPRequest
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

    static void createUserType(sol::table &c2);
    friend class chatterino::PluginController;

    // This is the key in the registry the private table it held at (if it exists)
    // This might be a null QString if the request has already been executed or
    // the table wasn't created yet.
    int timeout_ = 10'000;
    bool done = false;

    std::optional<sol::main_protected_function> cbSuccess;
    std::optional<sol::main_protected_function> cbError;
    std::optional<sol::main_protected_function> cbFinally;

public:
    // These functions are wrapped so data can be accessed more easily. When a call from Lua comes in:
    //  - the static wrapper function is called
    //  - it calls getOrError
    //  - and then the wrapped method

    /**
     * Sets the success callback
     *
     * @lua@param callback c2.HTTPCallback Function to call when the HTTP request succeeds
     * @exposed c2.HTTPRequest:on_success
     */
    void on_success(sol::main_protected_function func);

    /**
     * Sets the failure callback
     *
     * @lua@param callback c2.HTTPCallback Function to call when the HTTP request fails or returns a non-ok status
     * @exposed c2.HTTPRequest:on_error
     */
    void on_error(sol::main_protected_function func);

    /**
     * Sets the finally callback
     *
     * @lua@param callback fun(): nil Function to call when the HTTP request finishes
     * @exposed c2.HTTPRequest:finally
     */
    void finally(sol::main_protected_function func);

    /**
     * Sets the timeout
     *
     * @lua@param timeout integer How long in milliseconds until the times out
     * @exposed c2.HTTPRequest:set_timeout
     */
    void set_timeout(int timeout);

    /**
     * Sets the request payload
     *
     * @lua@param data string
     * @exposed c2.HTTPRequest:set_payload
     */
    void set_payload(QByteArray payload);

    /**
     * Sets a header in the request
     *
     * @lua@param name string
     * @lua@param value string
     * @exposed c2.HTTPRequest:set_header
     */
    void set_header(QByteArray name, QByteArray value);

    /**
     * Executes the HTTP request
     *
     * @exposed c2.HTTPRequest:execute
     */
    void execute(sol::this_state L);
    /**
     * @lua@return string
     * @exposed c2.HTTPRequest:__tostring
     */
    QString to_string();

    /**
     * Static functions
     */

    /**
     * Creates a new HTTPRequest
     *
     * @lua@param method c2.HTTPMethod Method to use
     * @lua@param url string Where to send the request to
     *
     * @lua@return c2.HTTPRequest
     * @exposed c2.HTTPRequest.create
     */
    static std::shared_ptr<HTTPRequest> create(sol::this_state L,
                                               NetworkRequestType method,
                                               QString url);
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api

namespace chatterino::lua {
StackIdx push(lua_State *L, std::shared_ptr<api::HTTPRequest> request);
}  // namespace chatterino::lua

#endif
