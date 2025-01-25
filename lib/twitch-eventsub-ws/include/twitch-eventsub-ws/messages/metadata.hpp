#pragma once

#include "twitch-eventsub-ws/errors.hpp"

#include <boost/json.hpp>

#include <optional>
#include <string>

namespace chatterino::eventsub::lib::messages {

/*
{
  "metadata": {
    "message_id": "40cc68b8-dc5b-a46e-0388-a7c9193eec5e",
    "message_type": "session_welcome",
    "message_timestamp": "2023-05-14T12:31:47.995298776Z"
    "subscription_type": "channel.unban",  // only included on message_type=notification
    "subscription_version": "1"  // only included on message_type=notification
  },
  "payload": ...
}
*/

/// json_transform=snake_case
struct Metadata {
    const std::string messageID;
    const std::string messageType;
    // TODO: should this be chronofied?
    const std::string messageTimestamp;

    const std::optional<std::string> subscriptionType;
    const std::optional<std::string> subscriptionVersion;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Metadata, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Metadata>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::messages
