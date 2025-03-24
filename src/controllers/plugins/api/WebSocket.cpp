#include "controllers/plugins/api/WebSocket.hpp"

#include <utility>

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "util/PostToThread.hpp"

namespace chatterino::lua::api {

class WebSocketListenerProxy final : public WebSocketListener
{
public:
    WebSocketListenerProxy(std::weak_ptr<WebSocket> target);

    void onClose(std::unique_ptr<WebSocketListener> self) override;
    void onBinaryMessage(QByteArray data) override;
    void onTextMessage(QByteArray data) override;

private:
    std::weak_ptr<WebSocket> target;
};

WebSocket::WebSocket() = default;

void WebSocket::createUserType(sol::table &c2)
{
    c2.new_usertype<WebSocket>(
        "WebSocket",
        sol::factories([](const QString &spec, sol::variadic_args args) {
            QUrl url(spec);
            if (url.scheme() != "wss" && url.scheme() != "ws")
            {
                throw std::runtime_error("Scheme must be wss:// or ws://");
            }

            WebSocketOptions opts{.url = url, .headers = {}};
            if (args.size() >= 1)
            {
                sol::table luaOpts = args[0];
                std::optional<sol::table> headers = luaOpts["headers"];
                if (headers)
                {
                    for (const auto &[k, v] : *headers)
                    {
                        opts.headers.emplace_back(k.as<std::string>(),
                                                  v.as<std::string>());
                    }
                }
            }

            auto self = std::make_shared<WebSocket>();
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
                cb();
            }
        }
    });
}

void WebSocketListenerProxy::onTextMessage(QByteArray data)
{
    auto target = this->target;
    runInGuiThread([target, data{std::move(data)}] {
        auto strong = target.lock();
        if (strong)
        {
            if (strong->onText)
            {
                strong->onText(data);
            }
        }
    });
}

void WebSocketListenerProxy::onBinaryMessage(QByteArray data)
{
    auto target = this->target;
    runInGuiThread([target, data{std::move(data)}] {
        auto strong = target.lock();
        if (strong)
        {
            if (strong->onBinary)
            {
                strong->onBinary(data);
            }
        }
    });
}

}  // namespace chatterino::lua::api

#endif
