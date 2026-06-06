// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/HighlightController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "controllers/highlights/types/Common.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/TwitchAccount.hpp"  // IWYU pragma: keep
#include "singletons/Settings.hpp"
#include "util/Variant.hpp"

namespace chatterino {

template <typename T>
concept SupportsValidityCheck = requires(T a) {
    { a.isValid() } -> std::same_as<bool>;
};

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

void rebuildSharedHighlights(Settings &settings,
                             std::vector<HighlightCheck> &checks)
{
    auto highlights = settings.sharedHighlights.readOnly();

    for (const auto &highlight : *highlights)
    {
        auto enabled = highlights::isEnabled(highlight);

        if (!enabled)
        {
            continue;
        }

        std::visit(
            [&checks](auto &&h) {
                auto check = h.buildCheck();
                if (check.cb)
                {
                    checks.emplace_back(std::move(check));
                }
            },
            highlight);
    }
}

}  // namespace

HighlightController::HighlightController(Settings &settings,
                                         AccountController *accounts)
{
    assert(accounts != nullptr);

    this->signalHolder_.managedConnect(
        accounts->twitch.currentUserChanged, [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because user swapped accounts";
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        accounts->twitch.currentUserNameChanged, [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because user name changed";
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        getSettings()->sharedHighlights.delayedItemsChanged, [this, &settings] {
            qCInfo(chatterinoHighlights)
                << "XXX: Rebuild checks because shared highlights changed";
            this->rebuildChecks(settings);
        });

    this->rebuildChecks(settings);
}

void HighlightController::rebuildChecks(Settings &settings)
{
    // Access checks for modification
    auto checks = this->checks_.access();
    checks->clear();

    rebuildSharedHighlights(settings, *checks);
}

std::pair<bool, HighlightResult> HighlightController::check(
    const MessageParseArgs &args, const std::vector<TwitchBadge> &twitchBadges,
    const QString &senderName, const QString &originalMessage,
    const MessageFlags &messageFlags, filters::RunContext runContext) const
{
    bool highlighted = false;
    auto result = HighlightResult::emptyResult();

    // Access for checking
    const auto checks = this->checks_.accessConst();

    auto currentUser = getApp()->getAccounts()->twitch.getCurrent();
    auto self = (senderName == currentUser->getUserName());

    for (const auto &check : *checks)
    {
        if (auto checkResult =
                check.cb(args, twitchBadges, senderName, originalMessage,
                         messageFlags, self, runContext);
            checkResult)
        {
            highlighted = true;

            // TODO TEMP XD
            result.ids.append(checkResult->ids);

            if (checkResult->alert)
            {
                if (!result.alert)
                {
                    result.alert = checkResult->alert;
                }
            }

            if (checkResult->playSound)
            {
                if (!result.playSound)
                {
                    result.playSound = checkResult->playSound;
                }
            }

            if (checkResult->customSoundUrl)
            {
                if (!result.customSoundUrl)
                {
                    result.customSoundUrl = checkResult->customSoundUrl;
                }
            }

            if (checkResult->color)
            {
                if (!result.color)
                {
                    result.color = checkResult->color;
                }
            }

            if (checkResult->showInMentions)
            {
                if (!result.showInMentions)
                {
                    result.showInMentions = checkResult->showInMentions;
                }
            }

            if (result.full())
            {
                // The final highlight result does not have room to add any more parameters, early out
                break;
            }
        }
    }

    return {highlighted, result};
}

}  // namespace chatterino
