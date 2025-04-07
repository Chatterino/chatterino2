#pragma once

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::session_welcome {

/*
{
  "metadata": ...
  "payload": {
    "session": {
      "id": "44f8cbce_c7ee958a",
      "status": "connected",
      "keepalive_timeout_seconds": 10,
      "reconnect_url": null,
      "connected_at": "2023-05-14T12:31:47.995262791Z"
    }
  }
}
*/

/// json_inner=session
struct Payload {
    std::string id;
    std::optional<std::string> reconnectURL;     // null in welcome message
    std::optional<int> keepaliveTimeoutSeconds;  // null in reconnect message
};

#include "twitch-eventsub-ws/payloads/session-welcome.inc"

}  // namespace chatterino::eventsub::lib::payload::session_welcome
