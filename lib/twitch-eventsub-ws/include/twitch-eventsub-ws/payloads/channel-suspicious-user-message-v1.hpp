#pragma once

#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_suspicious_user_message::
    v1 {

struct Event {
    // Broadcaster of the channel the message was sent in
    std::string broadcasterUserID;
    std::string broadcasterUserLogin;
    std::string broadcasterUserName;

    // User who sent the message
    std::string userID;
    std::string userLogin;
    std::string userName;

    // "none", "active_monitoring", "restricted"
    std::string lowTrustStatus;

    std::vector<std::string> sharedBanChannelIds;

    // "manual", "ban_evader_detector", or "shared_channel_ban"
    std::vector<std::string> types;

    // "unknown", "possible", "likely"
    std::string banEvasionEvaluation;
    // this event also has the ID in this message (hopefully we don't need it)
    chat::Message message;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-suspicious-user-message-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_suspicious_user_message::v1
