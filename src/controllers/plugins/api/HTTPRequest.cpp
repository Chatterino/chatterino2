#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPRequest.hpp"

#    include "Application.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/api/HTTPResponse.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "util/DebugCount.hpp"

#    include <lauxlib.h>
#    include <lua.h>
#    include <QChar>
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

void HTTPRequest::createUserType(lua_State *L, sol::table &c2)
{
    c2.new_usertype<HTTPRequest>(            //
        "HTTPRequest", sol::no_constructor,  //

        "on_success", &HTTPRequest::on_success,  //
        "on_error", &HTTPRequest::on_error,      //
        "finally", &HTTPRequest::finally,        //

        "set_timeout", &HTTPRequest::set_timeout,  //
        "set_payload", &HTTPRequest::set_payload,  //
        "set_header", &HTTPRequest::set_header,    //

        "execute", &HTTPRequest::execute,  //
        "create", [L](NetworkRequestType method, const std::string &url) {
            return HTTPRequest::create(L, method, QString::fromStdString(url));
        });
}

void HTTPRequest::on_success(sol::protected_function func)
{
    this->cbSuccess = {func};
}

void HTTPRequest::on_error(sol::protected_function func)
{
    this->cbError = {func};
}

void HTTPRequest::set_timeout(int timeout)
{
    this->timeout_ = timeout;
}

void HTTPRequest::finally(sol::protected_function func)
{
    this->cbFinally = {func};
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

HTTPRequest HTTPRequest::create(lua_State *L, NetworkRequestType method,
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
    return {ConstructorAccessTag{}, std::move(r), L};
}

void HTTPRequest::execute()
{
    this->done = true;
    std::move(this->req_)
        .onSuccess([this](const NetworkResult &res) {
            if (!this->cbSuccess.has_value())
            {
                return;
            }
            lua::StackGuard guard(this->state_);
            sol::state_view mainState(this->state_);
            sol::thread thread = sol::thread::create(mainState);
            sol::state_view threadstate = thread.state();
            sol::protected_function cb(threadstate, *this->cbSuccess);
            cb(HTTPResponse(res));
            this->cbSuccess = std::nullopt;
        })
        .onError([this](const NetworkResult &res) {
            if (!this->cbError.has_value())
            {
                return;
            }
            lua::StackGuard guard(this->state_);
            sol::state_view mainState(this->state_);
            sol::thread thread = sol::thread::create(mainState);
            sol::state_view threadstate = thread.state();
            sol::protected_function cb(threadstate, *this->cbError);
            cb(HTTPResponse(res));
            this->cbError = std::nullopt;
        })
        .finally([this]() {
            if (!this->cbFinally.has_value())
            {
                return;
            }
            lua::StackGuard guard(this->state_);
            sol::state_view mainState(this->state_);
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
                         NetworkRequest req, lua_State *state)
    : req_(std::move(req))
    , state_(state)
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
