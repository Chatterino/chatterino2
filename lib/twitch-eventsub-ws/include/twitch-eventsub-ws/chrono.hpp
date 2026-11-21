#pragma once

#include <boost/json.hpp>

#include <chrono>
#include <version>

#if __cpp_lib_chrono < 201907L
#    define CHATTERINO_USING_HOWARD_HINNANTS_DATE
#endif

namespace chatterino::eventsub::lib {

struct AsISO8601 {
};

boost::json::result_for<std::chrono::system_clock::time_point,
                        boost::json::value>::type
    tag_invoke(
        boost::json::try_value_to_tag<std::chrono::system_clock::time_point>,
        const boost::json::value &jvRoot, const AsISO8601 &);

}  // namespace chatterino::eventsub::lib
