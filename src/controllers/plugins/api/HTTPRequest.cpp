#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/HTTPRequest.hpp"

#    include "Application.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "common/network/NetworkRequest.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/api/HTTPResponse.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginController.hpp"
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
#    include <vector>

namespace chatterino::lua::api {

void HTTPRequest::createUserType(sol::table &c2)
{
    c2.new_usertype<HTTPRequest>(                              //
        "HTTPRequest", sol::no_constructor,                    //
        sol::meta_method::to_string, &HTTPRequest::to_string,  //

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

void HTTPRequest::on_success(sol::main_protected_function func)
{
    this->cbSuccess = std::make_optional(func);
}

void HTTPRequest::on_error(sol::main_protected_function func)
{
    this->cbError = std::make_optional(func);
}

void HTTPRequest::set_timeout(int timeout)
{
    this->timeout_ = timeout;
}

void HTTPRequest::finally(sol::main_protected_function func)
{
    this->cbFinally = std::make_optional(func);
}

void HTTPRequest::set_payload(QByteArray payload)
{
    this->req_ = std::move(this->req_).payload(payload);
}

// name and value may be random bytes
void HTTPRequest::set_header(QByteArray name, QByteArray value)
{
    this->req_ = std::move(this->req_).header(name, value);
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
    auto hack = this->weak_from_this();
    auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
    pl->httpRequests.push_back(this->shared_from_this());

    std::move(this->req_)
        .onSuccess([L, hack](const NetworkResult &res) {
            auto self = hack.lock();
            if (!self)
            {
                return;
            }
            if (!self->cbSuccess.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            (*self->cbSuccess)(HTTPResponse(res));
            self->cbSuccess = std::nullopt;
        })
        .onError([L, hack](const NetworkResult &res) {
            auto self = hack.lock();
            if (!self)
            {
                return;
            }
            if (!self->cbError.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            (*self->cbError)(HTTPResponse(res));
            self->cbError = std::nullopt;
        })
        .finally([L, hack]() {
            auto self = hack.lock();
            if (!self)
            {
                // this could happen if the plugin was deleted
                return;
            }
            auto *pl = getApp()->getPlugins()->getPluginByStatePtr(L);
            for (auto it = pl->httpRequests.begin();
                 it < pl->httpRequests.end(); it++)
            {
                if (*it == self)
                {
                    pl->httpRequests.erase(it);
                    break;
                }
            }

            if (!self->cbFinally.has_value())
            {
                return;
            }
            lua::StackGuard guard(L);
            (*self->cbFinally)();
            self->cbFinally = std::nullopt;
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

QString HTTPRequest::to_string()
{
    return "<HTTPRequest>";
}

}  // namespace chatterino::lua::api
#endif
