// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <memory>

namespace chatterino::ws::detail {
class WebSocketPoolImpl;
class WebSocketConnection;
}  // namespace chatterino::ws::detail

namespace chatterino {

/// A handle to a websocket connection.
///
/// Note that even though this handle only contains a weak pointer to the actual
/// connection, this handle controls the lifetime of the connection. Destroying
/// this handle will close the underlying connection gracefully (if possible).
/// It contains a weak pointer to avoid keeping the connection alive after the
/// parent pool has been destroyed.
class WebSocketHandle
{
public:
    WebSocketHandle() = default;
    WebSocketHandle(std::weak_ptr<ws::detail::WebSocketConnection> conn);
    ~WebSocketHandle();

    WebSocketHandle(const WebSocketHandle &) = delete;
    WebSocketHandle(WebSocketHandle &&) = default;
    WebSocketHandle &operator=(const WebSocketHandle &) = delete;
    WebSocketHandle &operator=(WebSocketHandle &&) = default;

    void sendText(const QByteArray &data);
    void sendBinary(const QByteArray &data);
    void close();

private:
    std::weak_ptr<ws::detail::WebSocketConnection> conn;
};

struct WebSocketListener {
    virtual ~WebSocketListener() = default;

    /// The WebSocket handshake completed successfully.
    ///
    /// This function is called from the websocket thread.
    virtual void onOpen() = 0;

    /// A text message was received.
    ///
    /// This function is called from the websocket thread.
    virtual void onTextMessage(QByteArray data) = 0;

    /// A binary message was received.
    ///
    /// This function is called from the websocket thread.
    virtual void onBinaryMessage(QByteArray data) = 0;

    /// The websocket was closed.
    ///
    /// This function is called from the websocket thread.
    /// @param self The allocated listener (i.e. `self.get() == this`). Be
    ///             careful where this is destroyed. Once `self` is destroyed,
    ///             the instance of this class will be destroyed.
    virtual void onClose(std::unique_ptr<WebSocketListener> self) = 0;
};

struct WebSocketOptions {
    QUrl url;
    std::vector<std::pair<std::string, std::string>> headers;
};

class WebSocketPool
{
public:
    WebSocketPool(QString shortName = {});
    ~WebSocketPool();

    [[nodiscard]] WebSocketHandle createSocket(
        WebSocketOptions options, std::unique_ptr<WebSocketListener> listener);

private:
    std::unique_ptr<ws::detail::WebSocketPoolImpl> impl;
    QString shortName;
};

}  // namespace chatterino
