// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace chatterino {

enum class HelixAnnouncementColor : std::uint8_t {
    Blue,
    Green,
    Orange,
    Purple,

    // this is the executor's chat color
    Primary,
};

enum class HelixClipError : std::uint8_t {
    Unknown,
    ClipsUnavailable,
    ClipsDisabled,
    ClipsRestricted,
    ClipsRestrictedCategory,
    UserNotAuthenticated,
};

enum class HelixStreamMarkerError : std::uint8_t {
    Unknown,
    UserNotAuthorized,
    UserNotAuthenticated,
};

enum class HelixAutoModMessageError : std::uint8_t {
    Unknown,
    MessageAlreadyProcessed,
    UserNotAuthenticated,
    UserNotAuthorized,
    MessageNotFound,
};

enum class HelixUpdateUserChatColorError : std::uint8_t {
    Unknown,
    UserMissingScope,
    InvalidColor,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixDeleteChatMessagesError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthenticated,
    UserNotAuthorized,
    MessageUnavailable,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixSendChatAnnouncementError : std::uint8_t {
    Unknown,
    UserMissingScope,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixAddChannelModeratorError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    TargetAlreadyModded,
    TargetIsVIP,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixRemoveChannelModeratorError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    TargetNotModded,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixAddChannelVIPError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixRemoveChannelVIPError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixUnbanUserError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    TargetNotBanned,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixStartRaidError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    CantRaidYourself,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixCancelRaidError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    NoRaidPending,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixUpdateChatSettingsError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    Forbidden,
    OutOfRange,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

/// Error type for Helix::updateChannel
///
/// Used in the /settitle and /setgame commands
enum class HelixUpdateChannelError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixBanUserError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    TargetBanned,
    CannotBanUser,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixWarnUserError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    CannotWarnUser,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixWhisperError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    NoVerifiedPhone,
    RecipientBlockedUser,
    WhisperSelf,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixGetChattersError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixGetModeratorsError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixListVIPsError : std::uint8_t {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    UserNotBroadcaster,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixSendShoutoutError : std::uint8_t {
    Unknown,
    // 400
    UserIsBroadcaster,
    BroadcasterNotLive,
    // 401
    UserNotAuthorized,
    UserMissingScope,

    Ratelimited,
};

enum class HelixUpdateShieldModeError : std::uint8_t {
    Unknown,
    UserMissingScope,
    MissingPermission,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixStartCommercialError : std::uint8_t {
    Unknown,
    TokenMustMatchBroadcaster,
    UserMissingScope,
    BroadcasterNotStreaming,
    MissingLengthParameter,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixGetGlobalBadgesError : std::uint8_t {
    Unknown,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixSendMessageError : std::uint8_t {
    Unknown,

    MissingText,
    BadRequest,
    Forbidden,
    MessageTooLarge,
    UserMissingScope,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixCreateEventSubSubscriptionError : std::uint8_t {
    BadRequest,
    Unauthorized,
    Forbidden,
    Conflict,
    Ratelimited,
    NoSession,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

}  // namespace chatterino
