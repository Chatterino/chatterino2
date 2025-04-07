#pragma once

#include "util/OnceFlag.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace chatterino::ws::detail {

class WebSocketConnection;

class WebSocketPoolImpl
{
public:
    WebSocketPoolImpl();
    ~WebSocketPoolImpl();

    WebSocketPoolImpl(const WebSocketPoolImpl &) = delete;
    WebSocketPoolImpl(WebSocketPoolImpl &&) = delete;
    WebSocketPoolImpl &operator=(const WebSocketPoolImpl &) = delete;
    WebSocketPoolImpl &operator=(WebSocketPoolImpl &&) = delete;

    void removeConnection(WebSocketConnection *conn);

    /// Attempts to shut down all connections by gracefully closing them.
    ///
    /// If the connections don't close within `timeout`, `false` is returned and
    /// this pool should be leaked.
    bool tryShutdown(std::chrono::milliseconds timeout);

    std::unique_ptr<std::thread> ioThread;
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::shared_ptr<WebSocketConnection>> connections;
    std::mutex connectionMutex;

    bool closing = false;
    int nextID = 1;

    OnceFlag shutdownFlag;
};

}  // namespace chatterino::ws::detail
