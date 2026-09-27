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
struct LowTrustUserHighlight;
struct UncategorizedNotificationHighlight;

// clang-format off
/// Variant of all types of highlights.
///
/// When you add a new built-in highlight, you must:
///  - Add it to HighlightController::billTinHighlights
///  - Add it to HighlightController::recreateMissingBillTinHighlights
///  - Update the BillTinHighlights test in HighlightController
///
/// When you add a new user-defined highlight, you must:
///  - Update the BillTinHighlights test in HighlightController
///  - Update the highlights/types/Common.cpp isUserDefined function
using AnyHighlight = std::variant<
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
    LowTrustUserHighlight,
    UncategorizedNotificationHighlight
    >;
// clang-format on

}  // namespace chatterino::highlights
