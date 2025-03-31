#pragma once

#include "common/FlagsEnum.hpp"

#include <magic_enum/magic_enum.hpp>

namespace chatterino {

enum class MessageFlag : std::int64_t {
    None = 0LL,
    System = (1LL << 0),
    Timeout = (1LL << 1),
    Highlighted = (1LL << 2),
    DoNotTriggerNotification = (1LL << 3),  // disable notification sound
    Centered = (1LL << 4),
    Disabled = (1LL << 5),
    DisableCompactEmotes = (1LL << 6),
    Collapsed = (1LL << 7),
    ConnectedMessage = (1LL << 8),
    DisconnectedMessage = (1LL << 9),
    Untimeout = (1LL << 10),
    PubSub = (1LL << 11),
    Subscription = (1LL << 12),
    DoNotLog = (1LL << 13),
    AutoMod = (1LL << 14),
    RecentMessage = (1LL << 15),
    Whisper = (1LL << 16),
    HighlightedWhisper = (1LL << 17),
    Debug = (1LL << 18),
    Similar = (1LL << 19),
    RedeemedHighlight = (1LL << 20),
    RedeemedChannelPointReward = (1LL << 21),
    ShowInMentions = (1LL << 22),
    FirstMessage = (1LL << 23),
    ReplyMessage = (1LL << 24),
    ElevatedMessage = (1LL << 25),
    SubscribedThread = (1LL << 26),
    CheerMessage = (1LL << 27),
    LiveUpdatesAdd = (1LL << 28),
    LiveUpdatesRemove = (1LL << 29),
    LiveUpdatesUpdate = (1LL << 30),
    /// The header of a message caught by AutoMod containing allow/disallow
    AutoModOffendingMessageHeader = (1LL << 31),
    /// The message caught by AutoMod containing the user who sent the message & its contents
    AutoModOffendingMessage = (1LL << 32),
    LowTrustUsers = (1LL << 33),
    /// The message is sent by a user marked as restricted with Twitch's "Low Trust"/"Suspicious User" feature
    RestrictedMessage = (1LL << 34),
    /// The message is sent by a user marked as monitor with Twitch's "Low Trust"/"Suspicious User" feature
    MonitoredMessage = (1LL << 35),
    /// The message is an ACTION message (/me)
    Action = (1LL << 36),
    /// The message is sent in a different source channel as part of a Shared Chat session
    SharedMessage = (1LL << 37),
    /// AutoMod message that showed up due to containing a blocked term in the channel
    AutoModBlockedTerm = (1LL << 38),
    /// The message is a full clear chat message (/clear)
    ClearChat = (1LL << 39),
    /// The message is built from EventSub
    EventSub = (1LL << 40),
    /// The message is a moderation action.
    /// Example messages that would count as moderation actions:
    ///  - forsen has been banned
    ///  - forsen deleted message from forsen
    ///  - forsen added "blockedterm" as a blocked term
    ///  - Your message is being checked by mods and has not been sent
    ModerationAction = (1LL << 41),
};
using MessageFlags = FlagsEnum<MessageFlag>;

}  // namespace chatterino

template <>
struct magic_enum::customize::enum_range<chatterino::MessageFlag> {
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr bool is_flags = true;
};
