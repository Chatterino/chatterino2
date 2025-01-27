#pragma once

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

/// json_transform=snake_case
struct Badge {
    std::string setID;
    std::string id;
    std::string info;
};

/// json_transform=snake_case
struct Cheermote {
    std::string prefix;
    int bits;
    int tier;
};

/// json_transform=snake_case
struct Emote {
    std::string id;
    std::string emoteSetID;
    std::string ownerID;
    std::vector<std::string> format;
};

/// json_transform=snake_case
struct Mention {
    std::string userID;
    std::string userName;
    std::string userLogin;
};

/// json_transform=snake_case
struct MessageFragment {
    std::string type;
    std::string text;
    std::optional<Cheermote> cheermote;
    std::optional<Emote> emote;
    std::optional<Mention> mention;
};

/// json_transform=snake_case
struct Message {
    std::string text;
    std::vector<MessageFragment> fragments;
};

/// json_transform=snake_case
struct Cheer {
    int bits;
};

/// json_transform=snake_case
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

/// json_transform=snake_case
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
    Message message;

    std::optional<Cheer> cheer;
    std::optional<Reply> reply;
    std::optional<std::string> channelPointsCustomRewardID;
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Badge, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Badge>, const boost::json::value &jvRoot);

boost::json::result_for<Cheermote, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Cheermote>, const boost::json::value &jvRoot);

boost::json::result_for<Emote, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Emote>, const boost::json::value &jvRoot);

boost::json::result_for<Mention, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Mention>, const boost::json::value &jvRoot);

boost::json::result_for<MessageFragment, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<MessageFragment>,
    const boost::json::value &jvRoot);

boost::json::result_for<Message, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Message>, const boost::json::value &jvRoot);

boost::json::result_for<Cheer, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Cheer>, const boost::json::value &jvRoot);

boost::json::result_for<Reply, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Reply>, const boost::json::value &jvRoot);

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::channel_chat_message::v1
