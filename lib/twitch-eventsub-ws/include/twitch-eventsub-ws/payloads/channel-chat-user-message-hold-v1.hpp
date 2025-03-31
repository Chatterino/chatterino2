#pragma once

#include "twitch-eventsub-ws/payloads/structured-message.hpp"
#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

namespace chatterino::eventsub::lib::payload::channel_chat_user_message_hold::
    v1 {

//  {"subscription":{"id":"94ab19f1-dd41-41e7-a5bc-f8f45bd4342a","status":"enabled","type":"channel.chat.user_message_hold","version":"1","condition":{"broadcaster_user_id":"117166826","user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQP515xn0yRxGvyU29dY9grBIGY2VsbC1j"},"created_at":"2025-03-01T10:40:14.527982426Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","user_id":"159849156","user_login":"bajlada","user_name":"BajLada","message_id":"e39e3c58-3d25-49fd-8c94-e776ef57a7f8","message":{"text":"penis penis penis","fragments":[{"type":"text","text":"penis penis penis","cheermote":null,"emote":null}]}}}

struct Event {
    // Broadcaster of the channel the message was sent in
    String broadcasterUserID;
    String broadcasterUserLogin;
    String broadcasterUserName;

    // User who sent the message
    String userID;
    String userLogin;
    String userName;

    String messageID;
    chat::Message message;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-chat-user-message-hold-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_chat_user_message_hold::v1
