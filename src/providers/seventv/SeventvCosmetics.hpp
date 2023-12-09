#pragma once

#include <magic_enum/magic_enum.hpp>

namespace chatterino::seventv {

enum class CosmeticKind {
    Badge,
    Paint,
    EmoteSet,

    INVALID,
};

}  // namespace chatterino::seventv

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::seventv::CosmeticKind>(
        chatterino::seventv::CosmeticKind value) noexcept
{
    using chatterino::seventv::CosmeticKind;
    switch (value)
    {
        case CosmeticKind::Badge:
            return "BADGE";
        case CosmeticKind::Paint:
            return "PAINT";
        case CosmeticKind::EmoteSet:
            return "EMOTE_SET";

        default:
            return default_tag;
    }
}
