#pragma once

#include <boost/json.hpp>

#include <chrono>

namespace chatterino::eventsub::lib {

struct AsISO8601 {
};

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    tag_invoke(
        boost::json::try_value_to_tag<std::chrono::system_clock::time_point>,
        const boost::json::value &jvRoot, const AsISO8601 &);

}  // namespace chatterino::eventsub::lib
