#include "controllers/highlights/types/AutomodCaughtHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/MessageBuilder.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

HighlightCheck AutomodCaughtHighlight::buildCheck() const
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

            if (!flags.has(MessageFlag::AutoModOffendingMessage))
            {
                return std::nullopt;
            }

            return HighlightResult{
                highlight.shouldHighlightTaskbar(),
                highlight.shouldPlaySound(),
                highlight.outcome.customSoundURL,
                highlight.outcome.backgroundColor,
                highlight.shouldShowInMentions(),
            };
        },
    };
}

}  // namespace chatterino::highlights
