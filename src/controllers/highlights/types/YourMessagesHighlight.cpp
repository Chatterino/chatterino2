// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/YourMessagesHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "providers/colors/ColorProvider.hpp"

namespace chatterino {

HighlightCheck YourMessagesHighlight::buildCheck() const
{
    auto showInMentions = this->shouldShowInMentions();

    return {
        [=](const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags,
            const auto self) -> std::optional<HighlightResult> {
            (void)originalMessage;  // unused
            (void)args;             // unused
            (void)senderName;       // unused
            (void)flags;            // unused
            (void)badges;           // unused

            if (!self)
            {
                return std::nullopt;
            }

            // Highlight color is provided by the ColorProvider and will be updated accordingly
            auto highlightColor = ColorProvider::instance().color(
                ColorType::SelfMessageHighlight);

            return HighlightResult{
                false, false, (QUrl) nullptr, highlightColor, showInMentions,
            };
        },
    };
}

}  // namespace chatterino
