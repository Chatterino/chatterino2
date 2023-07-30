#pragma once

#include <chrono>
#include <functional>
#include <memory>

class QLatin1String;
class QByteArray;
class QString;

namespace chatterino::ws {

// An internal handle to a websocket connection
using Handle = std::weak_ptr<void>;

class Connection;

class ClientPrivate;
/**
 * A generic websocket client
 * 
 * To use this client, entend from it and implement the event handlers.
 */
class Client
{
public:
    enum class CloseCode : uint16_t {
        Normal = 1000,
    };

    Client();
    virtual ~Client();

    Client(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(const Client &) = delete;
    Client &operator=(Client &&) = delete;

    /// Starts the websocket client and event-loop.
    /// This must be called at most once per client.
    void start();
    /// Stops the websocket client and event-loop and waits for it to exit.
    /// This must be called at most once per client.
    void stop();
    /// Stops the underlying event-loop without waiting for any exit/close.
    /// This must be called at most once per client.
    void forceStop();

    /// Adds a connection to `host`.
    /// Once the connection is open, `onConnectionOpen` will be called.
    /// If the connection failed (e.g. the host is unreachable), `onConnectionFail` will be called.
    ///
    /// @see #onConnectionOpen
    /// @see #onConnectionFail
    void addConnection(const QString &host);

    // TODO(Qt6): Use QByteArrayView
    /// Send `buf` as a text message through `conn`.
    bool sendText(const Connection &conn, const QByteArray &buf);
    /// Send `str` as a text message through `conn`.
    bool sendText(const Connection &conn, const QLatin1String &str);
    /// Send `str` with length `len` as a text message through `conn`.
    bool sendText(const Connection &conn, const char *str, size_t len);
    /// Close `conn`.
    void close(const Connection &conn, const QString &reason,
               CloseCode code = CloseCode::Normal);

    /// Runs `fn` after `duration` on the websocket thread.
    void runAfter(std::chrono::milliseconds duration,
                  const std::function<void()> &fn);

protected:
    /// A new connection was opened (after a call to `addConnection`).
    /// Users must track the connection and possible user-data themselves.
    ///
    /// @see #addConnection
    virtual void onConnectionOpen(const Connection &conn) = 0;
    /// A connection was closed.
    virtual void onConnectionClose(const Connection &conn) = 0;
    /// A connection attempt failed
    ///
    /// @see #addConnection
    virtual void onConnectionFail(QLatin1String reason) = 0;

    /// A text message was received.
    ///
    /// @param conn The connection the message was received on.
    /// @param data The text of the received message.
    virtual void onTextMessage(const Connection &conn,
                               const QByteArray &data) = 0;

private:
    std::unique_ptr<ClientPrivate> private_;

    friend class ClientPrivate;
};

/// An opaque handle to a connection.
class Connection
{
public:
    Connection(const Connection &) = default;
    Connection(Connection &&) = default;
    Connection &operator=(const Connection &) = default;
    Connection &operator=(Connection &&) = default;

    bool operator<(const Connection &other) const noexcept
    {
        return std::owner_less<Handle>{}(this->hdl_, other.hdl_);
    }

private:
    Connection(Handle &&hdl)
        : hdl_(std::move(hdl)){};

    Handle hdl_;

    friend class Client;
    friend class ClientPrivate;
};

}  // namespace chatterino::ws
