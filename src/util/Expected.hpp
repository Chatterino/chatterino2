#pragma once

#include <nonstd/expected.hpp>

class QString;

namespace chatterino {

template <typename T, typename E>
using Expected = nonstd::expected_lite::expected<T, E>;

template <typename T>
using ExpectedStr = Expected<T, QString>;

// convenience function from nonstd/expected.hpp
template <typename E>
constexpr nonstd::unexpected<std::decay_t<E>> makeUnexpected(E &&value)
{
    return nonstd::unexpected<std::decay_t<E>>(std::forward<E>(value));
}

}  // namespace chatterino
