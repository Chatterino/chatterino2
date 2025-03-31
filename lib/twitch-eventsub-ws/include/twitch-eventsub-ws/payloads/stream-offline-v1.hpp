#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::stream_offline::v1 {

struct Event {
    // The broadcaster's user ID
    std::string broadcasterUserID;
    // The broadcaster's user login
    std::string broadcasterUserLogin;
    // The broadcaster's user display name
    std::string broadcasterUserName;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/stream-offline-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::stream_offline::v1
