// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/AnnouncementsHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/AutomodCaughtHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/BadgeHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/ChannelPointsHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/FilterHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/FirstMessageHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/InvalidHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/MessageHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/SubscribedThreadHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/SubscriptionsHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/UncategorizedNotificationHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/UserHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/WatchStreakHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/WhispersHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/YourMessagesHighlight.hpp"  // IWYU pragma: export
#include "controllers/highlights/types/YourUsernameHighlight.hpp"  // IWYU pragma: export

#include <variant>

namespace chatterino::highlights {

// clang-format off
/// Variant of all highlights.
///
/// When you add a new built-in highlight, it must be added to HighlightController::billTinHighlights.
/// When you add a new user-defined highlight, it must be added to the HighlightControllerTest.BillTinHighlightsHighlightController test.
using AllHighlights = std::variant<
    InvalidHighlight,
    YourUsernameHighlight,
    WhispersHighlight,
    AnnouncementsHighlight,
    SubscriptionsHighlight,
    ChannelPointsHighlight,
    FirstMessageHighlight,
    SubscribedThreadHighlight,
    AutomodCaughtHighlight,
    WatchStreakHighlight,
    YourMessagesHighlight,
    MessageHighlight,
    UserHighlight,
    BadgeHighlight,
    FilterHighlight,
    UncategorizedNotificationHighlight
    >;
// clang-format on

}  // namespace chatterino::highlights
