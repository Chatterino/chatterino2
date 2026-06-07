// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/FlagsEnum.hpp"
#include "common/UniqueAccess.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "singletons/Settings.hpp"

#include <pajlada/settings.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QColor>
#include <QUrl>

#include <cstdint>
#include <utility>
#include <vector>

namespace chatterino {

class TwitchBadge;
struct MessageParseArgs;
class AccountController;
enum class MessageFlag : std::int64_t;
using MessageFlags = FlagsEnum<MessageFlag>;

namespace filters {

struct RunContext;

}  // namespace filters

class HighlightController final
{
public:
    HighlightController(Settings &settings, AccountController *accounts);

    /**
     * @brief Checks the given message parameters if it matches our internal checks, and returns a result
     **/
    [[nodiscard]] std::pair<bool, HighlightResult> check(
        const MessageParseArgs &args,
        const std::vector<TwitchBadge> &twitchBadges, const QString &senderName,
        const QString &originalMessage, const MessageFlags &messageFlags,
        filters::RunContext runContext) const;

    /// Return a set of built-in highlight IDs (e.g. {"whispers", "yourusername"}) that are missing from the user's list of highlights.
    /// This can happen if a user does some manual cursed surgery on their settings.json file.
    static QSet<QStringView> missingBillTinHighlights();

    /// Given a set of missing highlight IDs, recreates & adds them to the bottom of the user's highlights.
    static void recreateMissingBillTinHighlights(
        const QSet<QStringView> &missingHighlights);

private:
    /**
     * @brief rebuildChecks is called whenever some outside variable has been changed and our checks need to be updated
     *
     * rebuilds are always full, so if something changes we throw away all checks and build them all up from scratch
     **/
    void rebuildChecks(Settings &settings);

    UniqueAccess<std::vector<HighlightCheck>> checks_;

    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
