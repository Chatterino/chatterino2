#pragma once

#include "common/QLogging.hpp"

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <memory>

namespace chatterino {

class TwitchAccount;
struct ActionUser;

// Create timer using given ioService
template <typename Duration, typename Callback>
void runAfter(boost::asio::io_context &ioc, Duration duration, Callback cb)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(ioc);
    timer->expires_after(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec)
        {
            qCDebug(chatterinoPubSub)
                << "Error in runAfter:" << ec.message().c_str();
            return;
        }

        cb(timer);
    });
}

// Use provided timer
template <typename Duration, typename Callback>
void runAfter(std::shared_ptr<boost::asio::steady_timer> timer,
              Duration duration, Callback cb)
{
    timer->expires_after(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec)
        {
            qCDebug(chatterinoPubSub)
                << "Error in runAfter:" << ec.message().c_str();
            return;
        }

        cb(timer);
    });
}

}  // namespace chatterino
