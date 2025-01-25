#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::stream_online::v1 {

/// json_transform=snake_case
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

// DESERIALIZATION DEFINITION START
boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::stream_online::v1
