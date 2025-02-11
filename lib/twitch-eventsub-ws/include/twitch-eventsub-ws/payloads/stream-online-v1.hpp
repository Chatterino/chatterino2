#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::stream_online::v1 {

struct Event {
    // The ID of the stream
    const std::string id;

    // The broadcaster's user ID
    const std::string broadcasterUserID;
    // The broadcaster's user login
    const std::string broadcasterUserLogin;
    // The broadcaster's user display name
    const std::string broadcasterUserName;

    // The stream type (e.g. live, playlist, watch_party)
    const std::string type;

    // The timestamp at which the stream went online
    // TODO: chronofy?
    const std::string startedAt;
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

#include "twitch-eventsub-ws/payloads/stream-online-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::stream_online::v1
