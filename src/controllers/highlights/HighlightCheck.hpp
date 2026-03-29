// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/FlagsEnum.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

class QString;

namespace chatterino {

struct HighlightResult;
struct MessageParseArgs;
class TwitchBadge;

enum class MessageFlag : std::int64_t;
using MessageFlags = FlagsEnum<MessageFlag>;

struct HighlightCheck {
    using Checker = std::function<std::optional<HighlightResult>(
        const MessageParseArgs &args,
        const std::vector<TwitchBadge> &twitchBadges, const QString &senderName,
        const QString &originalMessage, const MessageFlags &messageFlags,
        bool self)>;
    Checker cb;
};

}  // namespace chatterino
