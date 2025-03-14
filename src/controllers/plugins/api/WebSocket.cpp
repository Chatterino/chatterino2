#include "controllers/plugins/api/WebSocket.hpp"

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
        "WebSocket", sol::factories([](const QString &spec) {
            QUrl url(spec);
            if (url.scheme() != "wss" && url.scheme() != "ws")
            {
                throw std::runtime_error("Scheme must be wss:// or ws://");
            }

            auto self = std::make_shared<WebSocket>();
            auto handle = getApp()->getPlugins()->webSocketPool().createSocket(
                {
                    .url = url,
                },
                std::make_unique<WebSocketListenerProxy>(self));

            self->handle = std::move(handle);
            return self;
        }),
        "on_close", &WebSocket::onClose,       //
        "on_text", &WebSocket::onText,         //
        "on_binary", &WebSocket::onBinary,     //
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
            if (strong->onClose)
            {
                strong->onClose();
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
