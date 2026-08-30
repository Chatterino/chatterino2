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
#include "providers/twitch/TwitchAccount.hpp"  // IWYU pragma: keep
#include "singletons/Settings.hpp"

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

/// Recreates the highlight of type Highlight if the highlight's ID is in the missingHighlights set
template <typename Highlight>
void recreateIfMissing(const std::unordered_set<QStringView> &missingHighlights)
{
    if (missingHighlights.contains(Highlight::ID))
    {
        qCInfo(LOG) << "Recreating missing highlight" << Highlight::ID;
        getSettings()->sharedHighlights.append(Highlight{});
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
    const HighlightCheck::Params &params) const
{
    bool highlighted = false;
    auto result = HighlightResult::emptyResult();

    // Access for checking
    const auto checks = this->checks_.accessConst();

    for (const auto &check : *checks)
    {
        if (auto checkResult = check.cb(params); checkResult)
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

            if (checkResult->sound.isValid())
            {
                if (result.sound.isEmpty())
                {
                    result.sound = checkResult->sound;
                }
            }

            if (checkResult->color)
            {
                if (checkResult->color->isValid())
                {
                    if (!result.color)
                    {
                        result.color = checkResult->color;
                    }
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

std::unordered_set<QStringView> HighlightController::billTinHighlights()
{
    using namespace chatterino::highlights;

    return {
        YourUsernameHighlight::ID,               //
        WhispersHighlight::ID,                   //
        AnnouncementsHighlight::ID,              //
        SubscriptionsHighlight::ID,              //
        ChannelPointsHighlight::ID,              //
        FirstMessageHighlight::ID,               //
        SubscribedThreadHighlight::ID,           //
        AutomodCaughtHighlight::ID,              //
        WatchStreakHighlight::ID,                //
        YourMessagesHighlight::ID,               //
        UncategorizedNotificationHighlight::ID,  //
    };
}

std::unordered_set<QStringView> HighlightController::missingBillTinHighlights()
{
    auto expectedHighlights = HighlightController::billTinHighlights();

    const auto highlights = getSettings()->sharedHighlights.readOnly();

    for (const auto &h : *highlights)
    {
        expectedHighlights.erase(highlights::getID(h));
    }

    return expectedHighlights;
}

void HighlightController::recreateMissingBillTinHighlights(
    const std::unordered_set<QStringView> &missingHighlights)
{
    using namespace chatterino::highlights;

    recreateIfMissing<YourUsernameHighlight>(missingHighlights);
    recreateIfMissing<WhispersHighlight>(missingHighlights);
    recreateIfMissing<AnnouncementsHighlight>(missingHighlights);
    recreateIfMissing<SubscriptionsHighlight>(missingHighlights);
    recreateIfMissing<ChannelPointsHighlight>(missingHighlights);
    recreateIfMissing<FirstMessageHighlight>(missingHighlights);
    recreateIfMissing<SubscribedThreadHighlight>(missingHighlights);
    recreateIfMissing<AutomodCaughtHighlight>(missingHighlights);
    recreateIfMissing<WatchStreakHighlight>(missingHighlights);
    recreateIfMissing<YourMessagesHighlight>(missingHighlights);
    recreateIfMissing<UncategorizedNotificationHighlight>(missingHighlights);
}

}  // namespace chatterino
