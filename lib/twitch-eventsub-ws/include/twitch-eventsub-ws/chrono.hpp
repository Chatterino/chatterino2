#pragma once

#include "twitch-eventsub-ws/date.h"

#include <boost/json.hpp>

#include <chrono>
#include <sstream>

namespace eventsub {

struct AsISO8601 {
};

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    tag_invoke(
        boost::json::try_value_to_tag<std::chrono::system_clock::time_point>,
        const boost::json::value &jvRoot, const AsISO8601 &);

}  // namespace eventsub
