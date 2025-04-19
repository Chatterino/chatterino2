#pragma once

#include <string_view>

namespace chatterino::eventsub::lib {

class Logger
{
public:
    virtual ~Logger() = default;
    virtual void debug(std::string_view msg) = 0;
    virtual void warn(std::string_view msg) = 0;
};

}  // namespace chatterino::eventsub::lib
