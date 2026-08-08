// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/YourUsernameHighlight.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "controllers/highlights/Sounds.hpp"
#include "providers/twitch/TwitchAccount.hpp"  // IWYU pragma: keep

namespace chatterino::highlights {

HighlightCheck YourUsernameHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;

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

    QUrl soundURL = this->outcome.getSoundURLWithDefault(H::SOUND_DEFAULT);

    return {
        [highlight = *this, regex, soundURL](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;        // unused
            (void)badges;      // unused
            (void)senderName;  // unused
            (void)flags;       // unused
            (void)runContext;  // unused

            if (self)
            {
                return std::nullopt;
            }

            if (!regex.match(originalMessage).hasMatch())
            {
                return std::nullopt;
            }

            return HighlightResult{
                .ids = {H::ID.toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .sound = soundURL,
                .color = highlight.outcome.getBackgroundColor(),
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

}  // namespace chatterino::highlights
