#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_suspicious_user_update::
    v1 {

struct Event {
    // Broadcaster of the channel
    std::string broadcasterUserID;
    std::string broadcasterUserLogin;
    std::string broadcasterUserName;

    // Affected user
    std::string userID;
    std::string userLogin;
    std::string userName;

    // Moderator who updated the user
    std::string moderatorUserID;
    std::string moderatorUserLogin;
    std::string moderatorUserName;

    // "none", "active_monitoring", or "restricted"
    std::string lowTrustStatus;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-suspicious-user-update-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_suspicious_user_update::v1
