#pragma once

#include "twitch-eventsub-ws/payloads/automod-message.hpp"
#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::automod_message_update::v2 {

struct Event {
    // Broadcaster of the channel the message was sent in
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // User who sent the message
    std::string userID;
    std::string userLogin;
    std::string userName;

    // Moderator who updated the message (possibly empty)
    std::string moderatorUserID;
    std::string moderatorUserLogin;
    std::string moderatorUserName;

    String messageID;
    chat::Message message;

    // "Approved", "Denied", or "Expired"
    std::string status;

    // TODO: use chrono?
    std::string heldAt;

    /// json_tag=reason
    std::variant<automod::AutomodReason, automod::BlockedTermReason> reason;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/automod-message-update-v2.inc"

}  // namespace chatterino::eventsub::lib::payload::automod_message_update::v2
