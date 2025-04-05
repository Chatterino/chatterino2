#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

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

    std::unique_ptr<std::thread> ioThread;
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;

    std::vector<std::shared_ptr<WebSocketConnection>> connections;
    std::mutex connectionMutex;

    bool closing = false;
    int nextID = 1;
};

}  // namespace chatterino::ws::detail
