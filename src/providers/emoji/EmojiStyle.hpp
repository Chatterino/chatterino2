// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace chatterino {

/// The available emoji styles in Chatterino
///
/// Each enum value has a "bitset value" defined so it can be used in a FlagsEnum to figure out
/// which emojis support which emoji style / set
enum class EmojiStyle : std::uint8_t {
    Twitter = 1 << 0,
    Facebook = 1 << 1,
    Apple = 1 << 2,
    Google = 1 << 3,
};

}  // namespace chatterino
