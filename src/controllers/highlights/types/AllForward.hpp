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
