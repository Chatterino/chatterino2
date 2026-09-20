// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <variant>

namespace chatterino::highlights {

struct YourUsernameHighlight;
struct WhispersHighlight;
struct AnnouncementsHighlight;
struct SubscriptionsHighlight;
struct InvalidHighlight;
struct ChannelPointsHighlight;
struct FirstMessageHighlight;
struct SubscribedThreadHighlight;
struct AutomodCaughtHighlight;
struct WatchStreakHighlight;
struct YourMessagesHighlight;
struct MessageHighlight;
struct FilterHighlight;
struct UserHighlight;
struct BadgeHighlight;
struct UncategorizedNotificationHighlight;

// clang-format off
/// Variant of all highlights.
///
/// When you add a new built-in highlight, it must be added to HighlightController::billTinHighlights and HighlightController::recreateMissingBillTinHighlights. Ensure the order of the latter makes sense. The BillTinHighlights test in HighlightController will also need to be updated.
/// When you add a new user-defined highlight, it must be added to the HighlightControllerTest.BillTinHighlightsHighlightController test, and to the highlights/types/Common.cpp isUserDefined function.
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
