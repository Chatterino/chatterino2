#pragma once

#include <fmt/format.h>

namespace chatterino {

template <typename... Args>
auto fS(Args &&... args) -> decltype(fmt::format(std::forward<Args>(args)...))
{
    return fmt::format(std::forward<Args>(args)...);
}

}  // namespace chatterino
