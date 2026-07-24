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
struct SubscribedThreadHighlight;
struct AutomodCaughtHighlight;
struct WatchStreakHighlight;
struct YourMessagesHighlight;
struct MessageHighlight;
struct FilterHighlight;
struct UserHighlight;
struct BadgeHighlight;
struct UncategorizedNotificationHighlight;

using AllHighlights = std::variant<     //
    YourUsernameHighlight,              //
    WhispersHighlight,                  //
    SubscriptionsHighlight,             //
    ChannelPointsHighlight,             //
    FirstMessageHighlight,              //
    SubscribedThreadHighlight,          //
    AutomodCaughtHighlight,             //
    WatchStreakHighlight,               //
    YourMessagesHighlight,              //
    MessageHighlight,                   //
    UserHighlight,                      //
    BadgeHighlight,                     //
    FilterHighlight,                    //
    UncategorizedNotificationHighlight  //
    >;

}  // namespace chatterino::highlights
