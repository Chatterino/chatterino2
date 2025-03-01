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
    Manual,
    BanEvaderDetector,
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
