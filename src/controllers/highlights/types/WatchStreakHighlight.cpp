#include "controllers/highlights/types/WatchStreakHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageBuilder.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

HighlightCheck WatchStreakHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;

    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;             // unused
            (void)badges;           // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused
            (void)runContext;       // unused

            if (true)
            {
                // TODO: Implement
                return std::nullopt;
            }

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
