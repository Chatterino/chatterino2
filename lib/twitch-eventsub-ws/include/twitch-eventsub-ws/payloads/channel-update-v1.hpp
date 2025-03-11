#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_update::v1 {

struct Event {
    // The broadcaster's user ID
    std::string broadcasterUserID;
    // The broadcaster's user login
    std::string broadcasterUserLogin;
    // The broadcaster's user display name
    std::string broadcasterUserName;

    // The channel's stream title
    std::string title;

    // The channel's broadcast language
    std::string language;

    // The channels category ID
    std::string categoryID;
    // The category name
    std::string categoryName;

    // A boolean identifying whether the channel is flagged as mature
    bool isMature;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-update-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_update::v1
