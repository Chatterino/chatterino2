#include "controllers/highlights/types/HypeChatHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageBuilder.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

HighlightCheck HypeChatHighlight::buildCheck() const
{
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
                highlight.outcome.alert.value_or(
                    HypeChatHighlight::ALERT_DEFAULT),
                highlight.outcome.playSound.value_or(
                    HypeChatHighlight::PLAY_SOUND_DEFAULT),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.outcome.showInMentions.value_or(
                    HypeChatHighlight::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

}  // namespace chatterino::highlights
