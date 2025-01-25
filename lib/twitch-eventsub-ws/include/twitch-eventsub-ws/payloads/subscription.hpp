#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::subscription {

/*
{
  "metadata": ...,
  "payload": {
    "subscription": {
      "id": "4aa632e0-fca3-590b-e981-bbd12abdb3fe",
      "status": "enabled",
      "type": "channel.ban",
      "version": "1",
      "condition": {
        "broadcaster_user_id": "74378979"
      },
      "transport": {
        "method": "websocket",
        "session_id": "38de428e_b11f07be"
      },
      "created_at": "2023-05-20T12:30:55.518375571Z",
      "cost": 0
    },
    ...
  }
}
*/

/// json_transform=snake_case
struct Transport {
    const std::string method;
    const std::string sessionID;
};

/// json_transform=snake_case
struct Subscription {
    const std::string id;
    const std::string status;
    const std::string type;
    const std::string version;

    // TODO: How do we map condition here? vector of key/value pairs?

    const Transport transport;

    // TODO: chronofy?
    const std::string createdAt;
    const int cost;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Transport, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Transport>, const boost::json::value &jvRoot);

boost::json::result_for<Subscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subscription>,
    const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::subscription
