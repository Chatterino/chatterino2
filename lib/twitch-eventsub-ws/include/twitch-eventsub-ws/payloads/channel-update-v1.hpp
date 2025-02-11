#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_update::v1 {

struct Event {
    // The broadcaster's user ID
    const std::string broadcasterUserID;
    // The broadcaster's user login
    const std::string broadcasterUserLogin;
    // The broadcaster's user display name
    const std::string broadcasterUserName;

    // The channel's stream title
    const std::string title;

    // The channel's broadcast language
    const std::string language;

    // The channels category ID
    const std::string categoryID;
    // The category name
    const std::string categoryName;

    // A boolean identifying whether the channel is flagged as mature
    const bool isMature;
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

#include "twitch-eventsub-ws/payloads/channel-update-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_update::v1
