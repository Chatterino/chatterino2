#include "controllers/plugins/api/WebSocket.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "util/PostToThread.hpp"

#    include <utility>

namespace chatterino::lua::api {

/// A WebSocket listener that dispatches events on the GUI thread to the user type.
class WebSocketListenerProxy final : public WebSocketListener
{
public:
    WebSocketListenerProxy(std::weak_ptr<WebSocket> target);

    void onClose(std::unique_ptr<WebSocketListener> self) override;
    void onBinaryMessage(QByteArray data) override;
    void onTextMessage(QByteArray data) override;
    void onOpen() override;

private:
    std::weak_ptr<WebSocket> target;
};

WebSocket::WebSocket() = default;

void WebSocket::createUserType(sol::table &c2, Plugin *plugin)
{
    c2.new_usertype<WebSocket>(
        "WebSocket",
        sol::factories([plugin](const QString &spec, sol::variadic_args args) {
            if (!plugin->hasNetworkPermission())
            {
                throw std::runtime_error(
                    "Plugin does not have permission to use websockets");
            }
            QUrl url(spec);
            if (url.scheme() != "wss" && url.scheme() != "ws")
            {
                throw std::runtime_error("Scheme must be wss:// or ws://");
            }

            auto self = std::make_shared<WebSocket>();
            self->plugin = plugin;

            WebSocketOptions opts{.url = url, .headers = {}};
            if (args.size() >= 1)
            {
                sol::table luaOpts = args[0];
                sol::optional<sol::table> headers = luaOpts["headers"];
                if (headers)
                {
                    for (const auto &[k, v] : *headers)
                    {
                        opts.headers.emplace_back(k.as<std::string>(),
                                                  v.as<std::string>());
                    }
                }
                sol::optional<sol::main_function> onOpen = luaOpts["on_open"];
                sol::optional<sol::main_function> onText = luaOpts["on_text"];
                sol::optional<sol::main_function> onBinary =
                    luaOpts["on_binary"];
                sol::optional<sol::main_function> onClose = luaOpts["on_close"];
                if (onOpen)
                {
                    self->onOpen = std::move(*onOpen);
                }
                if (onText)
                {
                    self->onText = std::move(*onText);
                }
                if (onBinary)
                {
                    self->onBinary = std::move(*onBinary);
                }
                if (onClose)
                {
                    self->onClose = std::move(*onClose);
                }
            }

            auto handle = getApp()->getPlugins()->webSocketPool().createSocket(
                opts, std::make_unique<WebSocketListenerProxy>(self));

            self->handle = std::move(handle);
            return self;
        }),
        // Note: These properties could be pointers to members, but Clang 18
        // specifically can't compile these - see https://github.com/ThePhD/sol2/issues/1581
        "on_close",
        sol::property(
            [](WebSocket &ws) {
                return ws.onClose;
            },
            [](WebSocket &ws, sol::main_function fn) {
                ws.onClose = std::move(fn);
            }),
        "on_text",
        sol::property(
            [](WebSocket &ws) {
                return ws.onText;
            },
            [](WebSocket &ws, sol::main_function fn) {
                ws.onText = std::move(fn);
            }),
        "on_binary",
        sol::property(
            [](WebSocket &ws) {
                return ws.onBinary;
            },
            [](WebSocket &ws, sol::main_function fn) {
                ws.onBinary = std::move(fn);
            }),
        "on_open",
        sol::property(
            [](WebSocket &ws) {
                return ws.onOpen;
            },
            [](WebSocket &ws, sol::main_function fn) {
                ws.onOpen = std::move(fn);
            }),
        "close", &WebSocket::close,            //
        "send_text", &WebSocket::sendText,     //
        "send_binary", &WebSocket::sendBinary  //
    );
}

void WebSocket::close()
{
    this->handle.close();
}

void WebSocket::sendText(const QByteArray &data)
{
    this->handle.sendText(data);
}

void WebSocket::sendBinary(const QByteArray &data)
{
    this->handle.sendBinary(data);
}

WebSocketListenerProxy::WebSocketListenerProxy(std::weak_ptr<WebSocket> target)
    : target(std::move(target))
{
}

void WebSocketListenerProxy::onClose(std::unique_ptr<WebSocketListener> self)
{
    runInGuiThread([this, lifetime{std::move(self)}] {
        auto strong = this->target.lock();
        if (strong)
        {
            // clear our object, so we can get GC'd
            auto cb = std::move(strong->onClose);
            strong->onText.reset();
            strong->onBinary.reset();
            if (cb)
            {
                loggedVoidCall(cb, u"WebSocket.on_close", strong->plugin);
            }
        }
    });
}

void WebSocketListenerProxy::onTextMessage(QByteArray data)
{
    auto target = this->target;
    runInGuiThread([target, data{std::move(data)}] {
        auto strong = target.lock();
        if (strong && strong->onText)
        {
            loggedVoidCall(strong->onText, u"WebSocket.on_text", strong->plugin,
                           data);
        }
    });
}

void WebSocketListenerProxy::onBinaryMessage(QByteArray data)
{
    auto target = this->target;
    runInGuiThread([target, data{std::move(data)}] {
        auto strong = target.lock();
        if (strong && strong->onBinary)
        {
            loggedVoidCall(strong->onBinary, u"WebSocket.on_binary",
                           strong->plugin, data);
        }
    });
}

void WebSocketListenerProxy::onOpen()
{
    auto target = this->target;
    runInGuiThread([target] {
        auto strong = target.lock();
        if (strong && strong->onOpen)
        {
            loggedVoidCall(strong->onOpen, u"WebSocket.on_open",
                           strong->plugin);
        }
    });
}

}  // namespace chatterino::lua::api

#endif
