#include "controllers/highlights/types/SubscriptionsHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageBuilder.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

HighlightCheck SubscriptionsHighlight::buildCheck() const
{
    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)badges;           // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused
            (void)runContext;       // unused

            if (!args.isSubscriptionMessage)
            {
                return std::nullopt;
            }

            return HighlightResult{
                highlight.outcome.alert.value_or(
                    SubscriptionsHighlight::ALERT_DEFAULT),
                highlight.outcome.playSound.value_or(
                    SubscriptionsHighlight::PLAY_SOUND_DEFAULT),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.outcome.showInMentions.value_or(
                    SubscriptionsHighlight::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

}  // namespace chatterino::highlights
