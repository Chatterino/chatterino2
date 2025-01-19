#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <memory>
#include <thread>

namespace chatterino {

class EventSub
{
public:
    EventSub();
    ~EventSub();

    void createConnection();

private:
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;
    std::unique_ptr<std::thread> thread;
};

}  // namespace chatterino
