#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPResponse.hpp"

#    include "common/network/NetworkResult.hpp"
#    include "util/DebugCount.hpp"

#    include <lauxlib.h>
#    include <sol/raii.hpp>
#    include <sol/types.hpp>

#    include <utility>

namespace chatterino::lua::api {

void HTTPResponse::createUserType(sol::table &c2)
{
    c2.new_usertype<HTTPResponse>(  //
        "HTTPResponse", sol::no_constructor,
        // metamethods
        // TODO: add a __tostring
        "data", &HTTPResponse::data,      //
        "status", &HTTPResponse::status,  //
        "error", &HTTPResponse::error     //
    );
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

QByteArray HTTPResponse::data()
{
    return this->result_.getData();
}

std::optional<int> HTTPResponse::status()
{
    return this->result_.status();
}

QString HTTPResponse::error()
{
    return this->result_.formatError();
}

}  // namespace chatterino::lua::api
#endif
