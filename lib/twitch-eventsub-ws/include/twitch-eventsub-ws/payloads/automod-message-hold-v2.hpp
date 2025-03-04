#pragma once

#include "twitch-eventsub-ws/payloads/automod-message.hpp"
#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::automod_message_hold::v2 {

struct Event {
    // Broadcaster of the channel the message was sent in
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // User who sent the message
    String userID;
    String userLogin;
    String userName;

    String messageID;
    chat::Message message;

    // TODO: use chrono?
    std::string heldAt;

    /// json_tag=reason
    std::variant<automod::AutomodReason, automod::BlockedTermReason> reason;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/automod-message-hold-v2.inc"

}  // namespace chatterino::eventsub::lib::payload::automod_message_hold::v2
