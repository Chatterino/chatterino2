#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace eventsub::payload::stream_offline::v1 {

/// json_transform=snake_case
struct Event {
    // The broadcaster's user ID
    const std::string broadcasterUserID;
    // The broadcaster's user login
    const std::string broadcasterUserLogin;
    // The broadcaster's user display name
    const std::string broadcasterUserName;
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

}  // namespace eventsub::payload::stream_offline::v1
