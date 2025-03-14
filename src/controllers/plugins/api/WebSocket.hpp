#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "singletons/WebSocketPool.hpp"

#    include <sol/protected_function.hpp>
#    include <sol/types.hpp>

namespace chatterino::lua::api {

/**
 * @lua@class c2.WebSocket
 */
class WebSocket
{
public:
    /**
     * Creates and connects to a WebSocket server. Only TLS connections are 
     * supported. Upon calling this, a connection is made immediately.
     *
     * @lua@param url string The URL to connect to. Must start with `wss://`.
     * @lua@return c2.WebSocket
     * @lua@nodiscard
     * @exposed c2.WebSocket.new
     */
    WebSocket();

    static void createUserType(sol::table &c2);

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

    friend class WebSocketListenerProxy;
};

}  // namespace chatterino::lua::api

#endif
