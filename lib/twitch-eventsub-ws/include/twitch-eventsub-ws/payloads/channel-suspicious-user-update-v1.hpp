#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/payloads/suspicious-users.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_suspicious_user_update::
    v1 {

struct Event {
    // Broadcaster of the channel
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // Affected user
    String userID;
    String userLogin;
    String userName;

    // Moderator who updated the user
    String moderatorUserID;
    String moderatorUserLogin;
    String moderatorUserName;

    suspicious_users::Status lowTrustStatus;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-suspicious-user-update-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_suspicious_user_update::v1
