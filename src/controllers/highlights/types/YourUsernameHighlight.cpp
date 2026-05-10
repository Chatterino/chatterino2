#include "controllers/highlights/types/YourUsernameHighlight.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "providers/twitch/TwitchAccount.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

namespace {

constexpr QStringView REGEX_START_BOUNDARY(u"(?:\\b|\\s|^)");
constexpr QStringView REGEX_END_BOUNDARY(u"(?:\\b|\\s|$)");

}  // namespace

HighlightCheck YourUsernameHighlight::buildCheck() const
{
    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();

    if (currentUser->isAnon())
    {
        return {};
    }

    auto currentUsername = currentUser->getUserName();

    if (currentUsername.isEmpty())
    {
        return {};
    }

    QRegularExpression regex(REGEX_START_BOUNDARY %
                                 QRegularExpression::escape(currentUsername) %
                                 REGEX_END_BOUNDARY,
                             QRegularExpression::UseUnicodePropertiesOption |
                                 QRegularExpression::CaseInsensitiveOption);

    return {
        [highlight = *this, regex](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)badges;           // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused
            (void)runContext;       // unused

            if (self)
            {
                return std::nullopt;
            }

            if (!regex.match(originalMessage).hasMatch())
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
