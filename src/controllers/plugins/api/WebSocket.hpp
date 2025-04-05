#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/websockets/WebSocketPool.hpp"

#    include <sol/protected_function.hpp>
#    include <sol/types.hpp>

namespace chatterino {
class Plugin;
}  // namespace chatterino

namespace chatterino::lua::api {

/**
 * @lua@class c2.WebSocket
 */
class WebSocket
{
public:
    /**
     * Creates and connects to a WebSocket server. Upon calling this, a
     * connection is made immediately.
     *
     * @lua@param url string The URL to connect to. Must start with `wss://` or `ws://`.
     * @lua@param options? { headers?: table<string, string>, on_close?: fun(), on_text?: fun(data: string), on_binary?: fun(data: string) } Additional options for the connection.
     * @lua@return c2.WebSocket
     * @lua@nodiscard
     * @exposed c2.WebSocket.new
     */
    WebSocket();

    static void createUserType(sol::table &c2, Plugin *plugin);

    /**
     * Closes the socket.
     *
     * @exposed c2.WebSocket:close
     */
    void close();
    /**
     * Sends a text message on the socket.
     *
     * @lua@param data string The text to send.
     * @exposed c2.WebSocket:send_text
     */
    void sendText(const QByteArray &data);
    /**
     * Sends a binary message on the socket.
     *
     * @lua@param data string The binary data to send.
     * @exposed c2.WebSocket:send_binary
     */
    void sendBinary(const QByteArray &data);

private:
    /**
     * @lua@field on_close fun()|nil Handler called when the socket is closed.
     */
    sol::main_function onClose;
    /**
     * @lua@field on_text fun(data: string)|nil Handler called when the socket receives a text message.
     */
    sol::main_function onText;
    /**
     * @lua@field on_binary fun(data: string)|nil Handler called when the socket receives a binary message.
     */
    sol::main_function onBinary;
    WebSocketHandle handle;
    // Note: this class lives inside the plugin -> this pointer will be valid.
    Plugin *plugin = nullptr;

    friend class WebSocketListenerProxy;
};

}  // namespace chatterino::lua::api

#endif
