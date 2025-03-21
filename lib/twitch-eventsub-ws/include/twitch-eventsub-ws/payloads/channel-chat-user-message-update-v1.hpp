#pragma once

#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_chat_user_message_update::
    v1 {

// message approved:
// {"subscription":{"id":"3a9fd3ec-f5b5-49d7-8624-2195b3ebfda9","status":"enabled","type":"channel.chat.user_message_update","version":"1","condition":{"broadcaster_user_id":"117166826","user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQBFnyoXrLQxiCmkHTvJ8VNBIGY2VsbC1j"},"created_at":"2025-03-01T11:29:49.532375428Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","user_id":"159849156","user_login":"bajlada","user_name":"BajLada","status":"approved","message_id":"5686ec52-e02c-4042-ac21-1dfd8cab0f9f","message":{"text":"penis ass penis","fragments":[{"type":"text","text":"penis ass penis","cheermote":null,"emote":null}]}}}

// message denied:
// {"subscription":{"id":"3a9fd3ec-f5b5-49d7-8624-2195b3ebfda9","status":"enabled","type":"channel.chat.user_message_update","version":"1","condition":{"broadcaster_user_id":"117166826","user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQBFnyoXrLQxiCmkHTvJ8VNBIGY2VsbC1j"},"created_at":"2025-03-01T11:29:49.532375428Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","user_id":"159849156","user_login":"bajlada","user_name":"BajLada","status":"denied","message_id":"fc90dc3b-0634-41c3-8365-f40719f076ab","message":{"text":"penis  ass penis","fragments":[{"type":"text","text":"penis  ","cheermote":null,"emote":null},{"type":"text","text":"ass penis","cheermote":null,"emote":null}]}}}

enum class Status : std::uint8_t {
    Approved,
    Denied,
    Invalid,
};

struct Event {
    // Broadcaster of the channel the message was sent in
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // User who sent the message
    String userID;
    String userLogin;
    String userName;

    Status status;

    String messageID;
    chat::Message message;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-chat-user-message-update-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_chat_user_message_update::v1
