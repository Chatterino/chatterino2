#pragma once

#include "debug/log.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "util/rapidjson-helpers.hpp"

#include <boost/asio.hpp>

#include <memory>

namespace chatterino {
namespace singletons {

struct ActionUser;

const rapidjson::Value &getArgs(const rapidjson::Value &data);

bool getCreatedByUser(const rapidjson::Value &data, ActionUser &user);

bool getTargetUser(const rapidjson::Value &data, ActionUser &user);

std::string Stringify(const rapidjson::Value &v);

rapidjson::Document CreateListenMessage(const std::vector<std::string> &topicsVec,
                                        std::shared_ptr<providers::twitch::TwitchAccount> account);
rapidjson::Document CreateUnlistenMessage(const std::vector<std::string> &topicsVec);

// Create timer using given ioService
template <typename Duration, typename Callback>
void RunAfter(boost::asio::io_service &ioService, Duration duration, Callback cb)
{
    auto timer = std::make_shared<boost::asio::steady_timer>(ioService);
    timer->expires_from_now(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec) {
            debug::Log("Error in RunAfter: {}", ec.message());
            return;
        }

        cb(timer);
    });
}

// Use provided timer
template <typename Duration, typename Callback>
void RunAfter(std::shared_ptr<boost::asio::steady_timer> timer, Duration duration, Callback cb)
{
    timer->expires_from_now(duration);

    timer->async_wait([timer, cb](const boost::system::error_code &ec) {
        if (ec) {
            debug::Log("Error in RunAfter: {}", ec.message());
            return;
        }

        cb(timer);
    });
}

}  // namespace singletons
}  // namespace chatterino
