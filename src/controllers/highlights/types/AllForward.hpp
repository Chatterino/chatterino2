// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <variant>

namespace chatterino {

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
struct SharedHighlight2;

using AllHighlights =
    std::variant<YourUsernameHighlight, WhispersHighlight,
                 SubscriptionsHighlight, ChannelPointsHighlight,
                 FirstMessageHighlight, HypeChatHighlight,
                 SubscribedThreadHighlight, AutomodCaughtHighlight,
                 WatchStreakHighlight, YourMessagesHighlight, SharedHighlight2>;

}  // namespace chatterino
