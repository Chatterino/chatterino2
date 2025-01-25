#pragma once

#include "twitch-eventsub-ws/errors.hpp"

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
    const std::string id;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::session_welcome
