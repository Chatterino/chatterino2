#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_update::v1 {

/// json_transform=snake_case
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

// DESERIALIZATION DEFINITION START
boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::channel_update::v1
