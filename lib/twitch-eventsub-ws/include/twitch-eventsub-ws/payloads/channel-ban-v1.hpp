#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <chrono>
#include <string>

namespace chatterino::eventsub::lib::payload::channel_ban::v1 {

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
    "event": {
      "banned_at": "2023-05-20T12:30:55.518375571Z",
      "broadcaster_user_id": "74378979",
      "broadcaster_user_login": "testBroadcaster",
      "broadcaster_user_name": "testBroadcaster",
      "ends_at": "2023-05-20T12:40:55.518375571Z",
      "is_permanent": false,
      "moderator_user_id": "29024944",
      "moderator_user_login": "CLIModerator",
      "moderator_user_name": "CLIModerator",
      "reason": "This is a test event",
      "user_id": "40389552",
      "user_login": "testFromUser",
      "user_name": "testFromUser"
    }
  }
}
*/

struct Event {
    // User ID (e.g. 117166826) of the user who's channel the event took place in
    std::string broadcasterUserID;
    // User Login (e.g. testaccount_420) of the user who's channel the event took place in
    std::string broadcasterUserLogin;
    // User Name (e.g. 테스트계정420) of the user who's channel the event took place in
    std::string broadcasterUserName;

    // User ID (e.g. 117166826) of the user who took the action
    std::string moderatorUserID;
    // User Login (e.g. testaccount_420) of the user who took the action
    std::string moderatorUserLogin;
    // User Name (e.g. 테스트계정420) of the user who took the action
    std::string moderatorUserName;

    // User ID (e.g. 117166826) of the user who was timed out or banned
    std::string userID;
    // User Login (e.g. testaccount_420) of the user who was timed out or banned
    std::string userLogin;
    // User Name (e.g. 테스트계정420) of the user who was timed out or banned
    std::string userName;

    // Reason given for the timeout or ban.
    // If no reason was specified, this string is empty
    std::string reason;

    // Set to true if this was a ban.
    // If this is false, this event describes a timeout
    bool isPermanent;

    // Time point when the timeout or ban took place
    std::chrono::system_clock::time_point bannedAt;

    // Time point when the timeout will end
    std::optional<std::chrono::system_clock::time_point> endsAt;

    // Returns the duration of the timeout
    // If this event describes a ban, the value returned won't make sense
    std::chrono::system_clock::duration timeoutDuration() const;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-ban-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_ban::v1
