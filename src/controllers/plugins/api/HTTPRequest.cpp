#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPRequest.hpp"

#    include "Application.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/plugins/api/HTTPResponse.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "util/DebugCount.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <QChar>
#    include <QLoggingCategory>
#    include <QRandomGenerator>
#    include <QUrl>
#    include <sol/forward.hpp>
#    include <sol/raii.hpp>
#    include <sol/state_view.hpp>
#    include <sol/table.hpp>
#    include <sol/types.hpp>

#    include <optional>
#    include <stdexcept>
#    include <utility>

namespace chatterino::lua::api {

void HTTPRequest::createUserType(sol::table &c2)
{
    c2.new_usertype<HTTPRequest>(            //
        "HTTPRequest", sol::no_constructor,  //

        "on_success", &HTTPRequest::on_success,  //
        "on_error", &HTTPRequest::on_error,      //
        "finally", &HTTPRequest::finally,        //

        "set_timeout", &HTTPRequest::set_timeout,  //
        "set_payload", &HTTPRequest::set_payload,  //
        "set_header", &HTTPRequest::set_header,    //
        "execute", &HTTPRequest::execute,          //

        "create", &HTTPRequest::create  //
    );
}

void HTTPRequest::on_success(sol::protected_function func)
{
    this->cbSuccess = std::make_optional(func);
}

void HTTPRequest::on_error(sol::protected_function func)
{
    this->cbError = std::make_optional(func);
}

void HTTPRequest::set_timeout(int timeout)
{
    this->timeout_ = timeout;
}

void HTTPRequest::finally(sol::protected_function func)
{
    this->cbFinally = std::make_optional(func);
}

void HTTPRequest::set_payload(const std::string &payload)
{
    this->req_ =
        std::move(this->req_).payload(QByteArray::fromStdString(payload));
}

// name and value may be random bytes
void HTTPRequest::set_header(std::string name, std::string value)
{
    this->req_ = std::move(this->req_)
                     .header(QByteArray::fromStdString(name),
                             QByteArray::fromStdString(value));
}

std::shared_ptr<HTTPRequest> HTTPRequest::create(sol::this_state L,
                                                 NetworkRequestType method,
                                                 QString url)
{
    auto parsedurl = QUrl(url);
    if (!parsedurl.isValid())
    {
        throw std::runtime_error(
            "cannot parse url (2nd argument of HTTPRequest.create, "
            "got invalid url in argument)");
    }
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    if (!pl->hasHTTPPermissionFor(parsedurl))
    {
        throw std::runtime_error(
            "Plugin does not have permission to send HTTP requests "
            "to this URL");
    }
    NetworkRequest r(parsedurl, method);
    return std::make_shared<HTTPRequest>(ConstructorAccessTag{}, std::move(r));
}

void HTTPRequest::execute(sol::this_state L)
{
    if (this->done)
    {
        throw std::runtime_error(
            "Cannot execute this c2.HTTPRequest, it was executed already!");
    }
    this->done = true;

    // this keeps the object alive even if Lua were to forget about it,
    auto keepalive = this->shared_from_this();
    std::move(this->req_)
        .onSuccess([this, L, keepalive](const NetworkResult &res) {
            if (!this->cbSuccess.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            sol::state_view mainState(L);
            sol::thread thread = sol::thread::create(mainState);
            sol::state_view threadstate = thread.state();
            sol::protected_function cb(threadstate, *this->cbSuccess);
            cb(HTTPResponse(res));
            this->cbSuccess = std::nullopt;
        })
        .onError([this, L, keepalive](const NetworkResult &res) {
            if (!this->cbError.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            sol::state_view mainState(L);
            sol::thread thread = sol::thread::create(mainState);
            sol::state_view threadstate = thread.state();
            sol::protected_function cb(threadstate, *this->cbError);
            cb(HTTPResponse(res));
            this->cbError = std::nullopt;
        })
        .finally([this, L, keepalive]() {
            if (!this->cbFinally.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            sol::state_view mainState(L);
            sol::thread thread = sol::thread::create(mainState);
            sol::state_view threadstate = thread.state();
            sol::protected_function cb(threadstate, *this->cbFinally);
            cb();
            this->cbFinally = std::nullopt;
        })
        .timeout(this->timeout_)
        .execute();
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

}  // namespace chatterino::lua::api
#endif
