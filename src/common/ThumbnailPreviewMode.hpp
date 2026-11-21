// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace chatterino {

enum class ThumbnailPreviewMode : std::uint8_t {
    DontShow,

    AlwaysShow,

    ShowOnShift,
};

constexpr std::optional<std::string_view> qmagicenumDisplayName(
    ThumbnailPreviewMode value) noexcept
{
    switch (value)
    {
        case ThumbnailPreviewMode::DontShow:
            return "Don't show";
        case ThumbnailPreviewMode::AlwaysShow:
            return "Always show";
        case ThumbnailPreviewMode::ShowOnShift:
            return "Hold shift";
    }
}

}  // namespace chatterino
