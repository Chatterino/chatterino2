// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace chatterino {

enum class HelixAnnouncementColor : std::uint8_t {
    Blue,
    Green,
    Orange,
    Purple,

    // this is the executor's chat color
    Primary,
};

}  // namespace chatterino
