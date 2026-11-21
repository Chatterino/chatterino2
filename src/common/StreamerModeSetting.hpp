// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace chatterino {

enum StreamerModeSetting : std::uint8_t {
    Disabled,
    Enabled,
    DetectStreamingSoftware,
};

constexpr std::optional<std::string_view> qmagicenumDisplayName(
    StreamerModeSetting value) noexcept
{
    switch (value)
    {
        case Disabled:
            return "Disabled";

        case Enabled:
            return "Enabled";

        case DetectStreamingSoftware:
            return "Automatic (Detect streaming software)";
    }
}

}  // namespace chatterino
