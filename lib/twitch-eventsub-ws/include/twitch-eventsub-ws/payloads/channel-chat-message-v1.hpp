#pragma once

#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <vector>

/*
{
  ...,
  "event": {
    "broadcaster_user_id": "117166826",
    "broadcaster_user_login": "testaccount_420",
    "broadcaster_user_name": "테스트계정420",
    "chatter_user_id": "100135110",
    "chatter_user_login": "streamelements",
    "chatter_user_name": "StreamElements",
    "message_id": "7732e85c-4543-464f-9d84-533e73f71459",
    "message": {
      "text": "https://youtu.be/v515yo0Ad_M",
      "fragments": [
        {
          "type": "text",
          "text": "https://youtu.be/v515yo0Ad_M",
          "cheermote": null,
          "emote": null,
          "mention": null
        }
      ]
    },
    "color": "#5B99FF",
    "badges": [
      {
        "set_id": "moderator",
        "id": "1",
        "info": ""
      },
      {
        "set_id": "partner",
        "id": "1",
        "info": ""
      }
    ],
    "message_type": "text",
    "cheer": null,
    "reply": null,
    "channel_points_custom_reward_id": null
  }
}
*/

namespace chatterino::eventsub::lib::payload::channel_chat_message::v1 {

struct Badge {
    std::string setID;
    std::string id;
    std::string info;
};

struct Cheer {
    int bits;
};

struct Reply {
    std::string parentMessageID;
    std::string parentUserID;
    std::string parentUserLogin;
    std::string parentUserName;
    std::string parentMessageBody;

    std::string threadMessageID;
    std::string threadUserID;
    std::string threadUserLogin;
    std::string threadUserName;
};

struct Event {
    // Broadcaster of the channel the message was sent in
    std::string broadcasterUserID;
    std::string broadcasterUserLogin;
    std::string broadcasterUserName;

    // User who sent the message
    std::string chatterUserID;
    std::string chatterUserLogin;
    std::string chatterUserName;

    // Color of the user who sent the message
    std::string color;

    std::vector<Badge> badges;

    std::string messageID;
    std::string messageType;
    chat::Message message;

    std::optional<Cheer> cheer;
    std::optional<Reply> reply;
    std::optional<std::string> channelPointsCustomRewardID;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-chat-message-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_chat_message::v1
