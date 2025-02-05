#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

#include <string>

namespace chatterino::eventsub::lib::payload::channel_moderate::v2 {

/* follower mode enabled
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"followers","followers":{"follow_duration_minutes":0},"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* follower mode disabled
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"followersoff","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* follower mode set to 7d
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"followers","followers":{"follow_duration_minutes":10080},"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Followers {
    int followDurationMinutes;
};

/* slow mode set to 30s
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"slow","followers":null,"slow":{"wait_time_seconds":30},"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* slow mode off
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"slowoff","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Slow {
    int waitTimeSeconds;
};

/* User is VIP'ed
{
  "subscription": {
    "id": "ef1d3e66-0001-4cdc-8e74-8746f9243704",
    "status": "enabled",
    "type": "channel.moderate",
    "version": "2",
    "condition": {
      "broadcaster_user_id": "11148817",
      "moderator_user_id": "117166826"
    },
    "transport": {
      "method": "websocket",
      "session_id": "AgoQBV1S6bGuR7e2hs04k4J1EhIGY2VsbC1j"
    },
    "created_at": "2025-02-01T11:55:30.114060889Z",
    "cost": 0
  },
  "event": {
    "broadcaster_user_id": "11148817",
    "broadcaster_user_login": "pajlada",
    "broadcaster_user_name": "pajlada",
    "source_broadcaster_user_id": null,
    "source_broadcaster_user_login": null,
    "source_broadcaster_user_name": null,
    "moderator_user_id": "11148817",
    "moderator_user_login": "pajlada",
    "moderator_user_name": "pajlada",
    "action": "vip",
    "followers": null,
    "slow": null,
    "vip": {
      "user_id": "159849156",
      "user_login": "bajlada",
      "user_name": "BajLada"
    },
    "unvip": null,
    "mod": null,
    "unmod": null,
    "ban": null,
    "unban": null,
    "timeout": null,
    "untimeout": null,
    "raid": null,
    "unraid": null,
    "delete": null,
    "automod_terms": null,
    "unban_request": null,
    "warn": null,
    "shared_chat_ban": null,
    "shared_chat_unban": null,
    "shared_chat_timeout": null,
    "shared_chat_untimeout": null,
    "shared_chat_delete": null
  }
}
*/

/// json_transform=snake_case
struct Vip {
    String userID;
    String userLogin;
    String userName;
};

/* user is unvipped
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unvip","followers":null,"slow":null,"vip":null,"unvip":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Unvip {
    String userID;
    String userLogin;
    String userName;
};

/* user is modded
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"mod","followers":null,"slow":null,"vip":null,"unvip":null,"mod":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Mod {
    std::string userID;
    std::string userLogin;
    std::string userName;
};

/* user is unmodded
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unmod","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Unmod {
    std::string userID;
    std::string userLogin;
    std::string userName;
};

/* user is banned with reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"ban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"this is the reason"},"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* user is banned without reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"ban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":""},"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Ban {
    std::string userID;
    std::string userLogin;
    std::string userName;
    // TODO: Verify that we handle null here
    std::string reason;
};

/* user is unbanned
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123"},"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Unban {
    std::string userID;
    std::string userLogin;
    std::string userName;
};

/* user is timed out without reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"timeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"","expires_at":"2025-02-01T12:11:02.684499409Z"},"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* user is timed out with reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"timeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"this is the reason","expires_at":"2025-02-01T12:11:15.859552916Z"},"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Timeout {
    std::string userID;
    std::string userLogin;
    std::string userName;
    // TODO: Verify that we handle null here
    std::string reason;
    // TODO: This should be a timestamp?
    std::string expiresAt;
};

/* user is untimeouted
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"untimeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123"},"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Untimeout {
    std::string userID;
    std::string userLogin;
    std::string userName;
};

/* channel is raided (from bajlada to pajlada)
{"subscription":{"id":"e7b45c7a-9b4d-4101-8d7d-92e8945c26fa","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"159849156","moderator_user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQEPRIfB3SQTCSkJM2zznvNxIGY2VsbC1j"},"created_at":"2025-02-01T12:12:45.685831769Z","cost":0},"event":{"broadcaster_user_id":"159849156","broadcaster_user_login":"bajlada","broadcaster_user_name":"BajLada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"159849156","moderator_user_login":"bajlada","moderator_user_name":"BajLada","action":"raid","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":{"user_id":"11148817","user_login":"pajlada","user_name":"pajlada","viewer_count":0},"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Raid {
    std::string userID;
    std::string userLogin;
    std::string userName;

    int viewerCount;
};

/* raid from bajlada to pajlada was cancelled
{"subscription":{"id":"e7b45c7a-9b4d-4101-8d7d-92e8945c26fa","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"159849156","moderator_user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQEPRIfB3SQTCSkJM2zznvNxIGY2VsbC1j"},"created_at":"2025-02-01T12:12:45.685831769Z","cost":0},"event":{"broadcaster_user_id":"159849156","broadcaster_user_login":"bajlada","broadcaster_user_name":"BajLada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"159849156","moderator_user_login":"bajlada","moderator_user_name":"BajLada","action":"unraid","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":{"user_id":"11148817","user_login":"pajlada","user_name":"pajlada"},"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Unraid {
    std::string userID;
    std::string userLogin;
    std::string userName;
};

/* message deleted
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"delete","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":{"user_id":"232490245","user_login":"namtheweebs","user_name":"NaMTheWeebs","message_id":"6c9bd28b-779b-4607-b66f-5c336e1ae29e","message_body":"bajlada raid NomNom"},"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Delete {
    std::string userID;
    std::string userLogin;
    std::string userName;
    std::string messageID;
    std::string messageBody;
};

/* automodded message approved
{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_permitted_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"permitted","terms":["cock cock cock penis sex cock"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* automodded message denied
{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_blocked_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"blocked","terms":["boobies"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}

{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_blocked_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"blocked","terms":["cock cock cock penis sex cock penis penis cock"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct AutomodTerms {
    // either add or remove
    std::string action;
    // either blocked or permitted
    std::string list;

    std::vector<std::string> terms;
    bool fromAutomod;
};

/* unban request approved
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"approve_unban_request","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":{"is_approved":true,"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","moderator_message":"you have been granted mercy"},"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* unban request denied
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"deny_unban_request","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":{"is_approved":false,"user_id":"975285839","user_login":"selenatormapguy","user_name":"selenatormapguy","moderator_message":""},"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct UnbanRequest {
    bool isApproved;

    std::string userID;
    std::string userLogin;
    std::string userName;

    std::string moderatorMessage;
};

/* freetext warn from chatterino
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"warn","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","reason":"this is a test warning from chatterino","chat_rules_cited":null},"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* warning from web ui
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"warn","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","reason":"and a custom reason","chat_rules_cited":["Rule 2"]},"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/// json_transform=snake_case
struct Warn {
    std::string userID;
    std::string userLogin;
    std::string userName;
    std::string reason;

    // TODO: Verify we handle null for this as an empty vector
    std::vector<std::string> chatRulesCited;
};

/// json_transform=snake_case
enum class Action : uint8_t {
    Ban,
    Timeout,
    Unban,
    Untimeout,
    Clear,
    Emoteonly,
    Emoteonlyoff,
    Followers,
    Followersoff,
    Uniquechat,
    Uniquechatoff,
    Slow,
    Slowoff,
    Subscribers,
    Subscribersoff,
    Unraid,
    /// json_rename=delete
    DeleteMessage,
    /// clangd currently "inherits" all future comments to all future enum constants
    /// so after using something like json_rename we need to ensure it doesn't spread
    Unvip,
    Vip,
    Raid,
    AddBlockedTerm,
    AddPermittedTerm,
    RemoveBlockedTerm,
    RemovePermittedTerm,
    Mod,
    Unmod,
    ApproveUnbanRequest,
    DenyUnbanRequest,
    Warn,
    SharedChatBan,
    SharedChatTimeout,
    SharedChatUnban,
    SharedChatUntimeout,
    SharedChatDelete,
};

/// json_transform=snake_case
struct Event {
    /// User ID (e.g. 117166826) of the user who's channel the event took place in
    String broadcasterUserID;
    /// User Login (e.g. testaccount_420) of the user who's channel the event took place in
    String broadcasterUserLogin;
    /// User Name (e.g. 테스트계정420) of the user who's channel the event took place in
    String broadcasterUserName;

    /// For Shared Chat events, the user ID (e.g. 117166826) of the user who's channel the event took place in
    std::optional<std::string> sourceBroadcasterUserID;
    /// For Shared Chat events, the user Login (e.g. testaccount_420) of the user who's channel the event took place in
    std::optional<std::string> sourceBroadcasterUserLogin;
    /// For Shared Chat events, the user Name (e.g. 테스트계정420) of the user who's channel the event took place in
    std::optional<std::string> sourceBroadcasterUserName;

    /// User ID (e.g. 117166826) of the user who took the action
    String moderatorUserID;
    /// User Login (e.g. testaccount_420) of the user who took the action
    String moderatorUserLogin;
    /// User Name (e.g. 테스트계정420) of the user who took the action
    String moderatorUserName;

    // TODO: enum?
    /// The action that took place (e.g. "warn" or "ban")
    Action action;

    std::optional<Followers> followers;
    std::optional<Slow> slow;
    std::optional<Vip> vip;
    std::optional<Unvip> unvip;
    std::optional<Unmod> unmod;
    std::optional<Ban> ban;
    std::optional<Unban> unban;
    std::optional<Timeout> timeout;
    std::optional<Untimeout> untimeout;
    std::optional<Raid> raid;
    std::optional<Unraid> unraid;
    /// json_rename=delete
    std::optional<Delete> deleteMessage;
    std::optional<AutomodTerms> automodTerms;
    std::optional<UnbanRequest> unbanRequest;
    std::optional<Warn> warn;
    std::optional<Ban> sharedChatBan;
    std::optional<Unban> sharedChatUnban;
    std::optional<Timeout> sharedChatTimeout;
    std::optional<Untimeout> sharedChatUntimeout;
    std::optional<Delete> sharedChatDelete;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Action, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Action>, const boost::json::value &jvRoot);

boost::json::result_for<Followers, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Followers>, const boost::json::value &jvRoot);

boost::json::result_for<Slow, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Slow>, const boost::json::value &jvRoot);

boost::json::result_for<Vip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Vip>, const boost::json::value &jvRoot);

boost::json::result_for<Unvip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unvip>, const boost::json::value &jvRoot);

boost::json::result_for<Mod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Mod>, const boost::json::value &jvRoot);

boost::json::result_for<Unmod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unmod>, const boost::json::value &jvRoot);

boost::json::result_for<Ban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Ban>, const boost::json::value &jvRoot);

boost::json::result_for<Unban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unban>, const boost::json::value &jvRoot);

boost::json::result_for<Timeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Timeout>, const boost::json::value &jvRoot);

boost::json::result_for<Untimeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Untimeout>, const boost::json::value &jvRoot);

boost::json::result_for<Raid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Raid>, const boost::json::value &jvRoot);

boost::json::result_for<Unraid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unraid>, const boost::json::value &jvRoot);

boost::json::result_for<Delete, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Delete>, const boost::json::value &jvRoot);

boost::json::result_for<AutomodTerms, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<AutomodTerms>,
    const boost::json::value &jvRoot);

boost::json::result_for<UnbanRequest, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<UnbanRequest>,
    const boost::json::value &jvRoot);

boost::json::result_for<Warn, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Warn>, const boost::json::value &jvRoot);

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace chatterino::eventsub::lib::payload::channel_moderate::v2
