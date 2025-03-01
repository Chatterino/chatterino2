#pragma once

#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/payloads/suspicious-users.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_suspicious_user_message::
    v1 {

struct Event {
    // Broadcaster of the channel the message was sent in
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // User who sent the message
    String userID;
    String userLogin;
    String userName;

    suspicious_users::Status lowTrustStatus;

    std::vector<String> sharedBanChannelIds;

    std::vector<suspicious_users::Type> types;

    suspicious_users::BanEvasionEvaluation banEvasionEvaluation;
    // this event also has the ID in this message (hopefully we don't need it)
    chat::Message message;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-suspicious-user-message-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_suspicious_user_message::v1
