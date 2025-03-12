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

struct Transport {
    std::string method;
    std::string sessionID;
};

struct Subscription {
    std::string id;
    std::string status;
    std::string type;
    std::string version;

    // TODO: How do we map condition here? vector of key/value pairs?

    Transport transport;

    // TODO: chronofy?
    std::string createdAt;
    int cost;
};

#include "twitch-eventsub-ws/payloads/subscription.inc"

}  // namespace chatterino::eventsub::lib::payload::subscription
