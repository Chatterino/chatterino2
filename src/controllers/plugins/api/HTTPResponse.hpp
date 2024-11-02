#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkResult.hpp"

#    include <lua.h>
#    include <sol/sol.hpp>

#    include <memory>

namespace chatterino {
class PluginController;
}  // namespace chatterino

namespace chatterino::lua::api {
// NOLINTBEGIN(readability-identifier-naming)

/**
 * @lua@class c2.HTTPResponse
 */
class HTTPResponse
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
    static void createUserType(sol::table &c2);
    friend class chatterino::PluginController;

public:
    /**
     * Returns the data. This is not guaranteed to be encoded using any
     * particular encoding scheme. It's just the bytes the server returned.
     * 
     * @lua@return string
     * @lua@nodiscard
     * @exposed c2.HTTPResponse:data
     */
    QByteArray data();

    /**
     * Returns the status code.
     *
     * @lua@return number|nil
     * @lua@nodiscard
     * @exposed c2.HTTPResponse:status
     */
    std::optional<int> status();

    /**
     * A somewhat human readable description of an error if such happened
     *
     * @lua@return string
     * @lua@nodiscard
     * @exposed c2.HTTPResponse:error
     */
    QString error();

    /**
     * @lua@return string
     * @lua@nodiscard
     * @exposed c2.HTTPResponse:__tostring
     */
    QString to_string();
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
#endif
