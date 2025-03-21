#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::stream_online::v1 {

struct Event {
    // The ID of the stream
    std::string id;

    // The broadcaster's user ID
    std::string broadcasterUserID;
    // The broadcaster's user login
    std::string broadcasterUserLogin;
    // The broadcaster's user display name
    std::string broadcasterUserName;

    // The stream type (e.g. live, playlist, watch_party)
    std::string type;

    // The timestamp at which the stream went online
    // TODO: chronofy?
    std::string startedAt;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/stream-online-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::stream_online::v1
