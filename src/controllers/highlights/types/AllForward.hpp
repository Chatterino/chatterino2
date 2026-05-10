// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <variant>

namespace chatterino::highlights {

struct YourUsernameHighlight;
struct WhispersHighlight;
struct SubscriptionsHighlight;
struct ChannelPointsHighlight;
struct FirstMessageHighlight;
struct HypeChatHighlight;
struct SubscribedThreadHighlight;
struct AutomodCaughtHighlight;
struct WatchStreakHighlight;
struct YourMessagesHighlight;
struct MessageHighlight;
struct FilterHighlight;
struct UserHighlight;
struct BadgeHighlight;

using AllHighlights =
    std::variant<YourUsernameHighlight, WhispersHighlight,
                 SubscriptionsHighlight, ChannelPointsHighlight,
                 FirstMessageHighlight, HypeChatHighlight,
                 SubscribedThreadHighlight, AutomodCaughtHighlight,
                 WatchStreakHighlight, YourMessagesHighlight, MessageHighlight,
                 UserHighlight, BadgeHighlight, FilterHighlight>;

}  // namespace chatterino::highlights
