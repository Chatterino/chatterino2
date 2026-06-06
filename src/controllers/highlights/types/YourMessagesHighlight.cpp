// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/YourMessagesHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

namespace chatterino::highlights {

HighlightCheck YourMessagesHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;

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

            // User has defined a color: std::shared_ptr<QColor>("#ff00ff")
            // User has not defined a color, should use default: std::shared_ptr<QColor> = {}; // unset shared ptr
            // User wants NO color, should fall through: std::shared_ptr<QColor> = std::shared_ptr<QColor>({}) // invalid QColor

            return HighlightResult{
                .ids = {H::ID.toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .playSound =
                    highlight.outcome.playSound.value_or(H::PLAY_SOUND_DEFAULT),
                .customSoundUrl = highlight.outcome.customSoundURL,
                .color = highlight.outcome.backgroundColor,
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

}  // namespace chatterino::highlights
