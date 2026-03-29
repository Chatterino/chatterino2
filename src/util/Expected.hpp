// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <version>

#if __cpp_lib_expected >= 202202L
#    include <expected>
#else
#    define CHATTERINO_USING_NONSTD_EXPECTED
#    include <nonstd/expected.hpp>
#endif

#include <type_traits>

class QString;

namespace chatterino {

#if __cpp_lib_expected >= 202202L
template <typename T, typename E>
using Expected = std::expected<T, E>;

// convenience function from nonstd/expected.hpp
template <typename E>
constexpr std::unexpected<std::decay_t<E>> makeUnexpected(E &&value)
{
    return std::unexpected<std::decay_t<E>>(std::forward<E>(value));
}
#else
template <typename T, typename E>
using Expected = nonstd::expected_lite::expected<T, E>;

// convenience function from nonstd/expected.hpp
template <typename E>
constexpr nonstd::unexpected<std::decay_t<E>> makeUnexpected(E &&value)
{
    return nonstd::unexpected<std::decay_t<E>>(std::forward<E>(value));
}
#endif

template <typename T>
using ExpectedStr = Expected<T, QString>;

}  // namespace chatterino
