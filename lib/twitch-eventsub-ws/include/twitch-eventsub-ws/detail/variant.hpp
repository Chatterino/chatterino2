#pragma once

#include <string_view>

namespace chatterino::eventsub::lib::detail {

template <typename T>
consteval std::string_view fieldFor()
{
    return T::TAG;
}

template <typename T>
consteval std::string_view fieldFor()
    requires requires { T::FIELD; }
{
    return T::FIELD;
}

}  // namespace chatterino::eventsub::lib::detail
