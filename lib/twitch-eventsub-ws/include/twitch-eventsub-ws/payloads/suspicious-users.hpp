#pragma once

#include <boost/json.hpp>

#include <cstdint>

namespace chatterino::eventsub::lib::suspicious_users {

/// default=None
enum class Status : std::uint8_t {
    None,
    ActiveMonitoring,
    Restricted,
};

/// default=Unknown
enum class Type : std::uint8_t {
    Unknown,
    /// The user was marked as Low Trust by a moderator
    /// json_extra_enum_constant_names=manually_added
    Manual,
    /// The user was detected as a ban evader
    BanEvaderDetector,
    /// The user is banned in a channel the channel shares bans with
    SharedChannelBan
};

/// default=Unknown
enum class BanEvasionEvaluation : std::uint8_t {
    Unknown,
    Possible,
    Likely,
};

#include "twitch-eventsub-ws/payloads/suspicious-users.inc"

}  // namespace chatterino::eventsub::lib::suspicious_users
