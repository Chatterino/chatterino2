#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "common/network/NetworkResult.hpp"

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
     * @exposed HTTPResponse:data
     */
    QByteArray data();

    /**
     * Returns the status code.
     *
     * @exposed HTTPResponse:status
     */
    std::optional<int> status();

    /**
     * A somewhat human readable description of an error if such happened
     * @exposed HTTPResponse:error
     */
    QString error();

    /**
     * @lua@return string
     * @exposed HTTPResponse:__tostring
     */
    QString to_string();
};

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api
#endif
