#pragma once

#include "debug/Log.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <memory>

namespace chatterino {
namespace providers {
namespace twitch {

struct ActionUser;

const rapidjson::Value &getArgs(const rapidjson::Value &data);

bool getCreatedByUser(const rapidjson::Value &data, ActionUser &user);

bool getTargetUser(const rapidjson::Value &data, ActionUser &user);

rapidjson::Document createListenMessage(const std::vector<std::string> &topicsVec,
                                        std::shared_ptr<providers::twitch::TwitchAccount> account);
rapidjson::Document createUnlistenMessage(const std::vector<std::string> &topicsVec);

// Create timer using given ioService
template <typename Duration, typename Callback>
void runAfter(boost::asio::io_service &ioService, Duration duration, Callback cb)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(ioService);
    timer->expires_from_now(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec) {
            debug::Log("Error in runAfter: {}", ec.message());
            return;
        }

        cb(timer);
    });
}

// Use provided timer
template <typename Duration, typename Callback>
void runAfter(std::shared_ptr<boost::asio::steady_timer> timer, Duration duration, Callback cb)
{
    timer->expires_from_now(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec) {
            debug::Log("Error in runAfter: {}", ec.message());
            return;
        }

        cb(timer);
    });
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
