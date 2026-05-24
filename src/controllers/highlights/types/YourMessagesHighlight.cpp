// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/YourMessagesHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

namespace chatterino::highlights {

HighlightCheck YourMessagesHighlight::buildCheck() const
{
    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)originalMessage;  // unused
            (void)args;             // unused
            (void)senderName;       // unused
            (void)flags;            // unused
            (void)badges;           // unused
            (void)runContext;       // unused

            if (!self)
            {
                return std::nullopt;
            }

            return HighlightResult{
                false,
                false,
                (QUrl) nullptr,
                highlight.outcome.backgroundColor,
                highlight.outcome.showInMentions.value_or(
                    YourMessagesHighlight::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

}  // namespace chatterino::highlights
