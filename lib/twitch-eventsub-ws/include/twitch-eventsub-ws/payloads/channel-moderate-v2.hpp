#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"
#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

#include <chrono>
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

struct Followers {
    static constexpr std::string_view TAG = "followers";

    int followDurationMinutes;
};

struct FollowersOff {
    static constexpr std::string_view TAG = "followersoff";
};

/* slow mode set to 30s
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"slow","followers":null,"slow":{"wait_time_seconds":30},"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* slow mode off
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"slowoff","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Slow {
    static constexpr std::string_view TAG = "slow";

    int waitTimeSeconds;
};

struct SlowOff {
    static constexpr std::string_view TAG = "slowoff";
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

struct Vip {
    static constexpr std::string_view TAG = "vip";

    String userID;
    String userLogin;
    String userName;
};

/* user is unvipped
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unvip","followers":null,"slow":null,"vip":null,"unvip":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Unvip {
    static constexpr std::string_view TAG = "unvip";

    String userID;
    String userLogin;
    String userName;
};

/* user is modded
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"mod","followers":null,"slow":null,"vip":null,"unvip":null,"mod":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Mod {
    static constexpr std::string_view TAG = "mod";

    String userID;
    String userLogin;
    String userName;
};

/* user is unmodded
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unmod","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada"},"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Unmod {
    static constexpr std::string_view TAG = "unmod";

    String userID;
    String userLogin;
    String userName;
};

/* user is banned with reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"ban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"this is the reason"},"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* user is banned without reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"ban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":""},"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Ban {
    static constexpr std::string_view TAG = "ban";

    String userID;
    String userLogin;
    String userName;
    String reason;
};
struct SharedChatBan : public Ban {
    static constexpr std::string_view TAG = "shared_chat_ban";
};

/* user is unbanned
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"unban","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123"},"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Unban {
    static constexpr std::string_view TAG = "unban";

    String userID;
    String userLogin;
    String userName;
};
struct SharedChatUnban : public Unban {
    static constexpr std::string_view TAG = "shared_chat_unban";
};

/* user is timed out without reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"timeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"","expires_at":"2025-02-01T12:11:02.684499409Z"},"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* user is timed out with reason
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"timeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123","reason":"this is the reason","expires_at":"2025-02-01T12:11:15.859552916Z"},"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Timeout {
    static constexpr std::string_view TAG = "timeout";

    String userID;
    String userLogin;
    String userName;
    String reason;
    std::chrono::system_clock::time_point expiresAt;
};
struct SharedChatTimeout : public Timeout {
    static constexpr std::string_view TAG = "shared_chat_timeout";
};

/* user is untimeouted
{"subscription":{"id":"86d99e53-2837-40cf-bc6e-c6e00698919c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQZgfnyKxXQ32hpzCWF4aCGBIGY2VsbC1j"},"created_at":"2025-02-01T12:02:16.005321321Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"untimeout","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":{"user_id":"70948394","user_login":"weeb123","user_name":"WEEB123"},"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Untimeout {
    static constexpr std::string_view TAG = "untimeout";

    String userID;
    String userLogin;
    String userName;
};
struct SharedChatUntimeout : public Untimeout {
    static constexpr std::string_view TAG = "shared_chat_untimeout";
};

/* channel is raided (from bajlada to pajlada)
{"subscription":{"id":"e7b45c7a-9b4d-4101-8d7d-92e8945c26fa","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"159849156","moderator_user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQEPRIfB3SQTCSkJM2zznvNxIGY2VsbC1j"},"created_at":"2025-02-01T12:12:45.685831769Z","cost":0},"event":{"broadcaster_user_id":"159849156","broadcaster_user_login":"bajlada","broadcaster_user_name":"BajLada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"159849156","moderator_user_login":"bajlada","moderator_user_name":"BajLada","action":"raid","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":{"user_id":"11148817","user_login":"pajlada","user_name":"pajlada","viewer_count":0},"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Raid {
    static constexpr std::string_view TAG = "raid";

    String userID;
    String userLogin;
    String userName;

    int viewerCount;
};

/* raid from bajlada to pajlada was cancelled
{"subscription":{"id":"e7b45c7a-9b4d-4101-8d7d-92e8945c26fa","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"159849156","moderator_user_id":"159849156"},"transport":{"method":"websocket","session_id":"AgoQEPRIfB3SQTCSkJM2zznvNxIGY2VsbC1j"},"created_at":"2025-02-01T12:12:45.685831769Z","cost":0},"event":{"broadcaster_user_id":"159849156","broadcaster_user_login":"bajlada","broadcaster_user_name":"BajLada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"159849156","moderator_user_login":"bajlada","moderator_user_name":"BajLada","action":"unraid","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":{"user_id":"11148817","user_login":"pajlada","user_name":"pajlada"},"delete":null,"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Unraid {
    static constexpr std::string_view TAG = "unraid";

    String userID;
    String userLogin;
    String userName;
};

/* message deleted
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"delete","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":{"user_id":"232490245","user_login":"namtheweebs","user_name":"NaMTheWeebs","message_id":"6c9bd28b-779b-4607-b66f-5c336e1ae29e","message_body":"bajlada raid NomNom"},"automod_terms":null,"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Delete {
    static constexpr std::string_view TAG = "delete";

    String userID;
    String userLogin;
    String userName;
    String messageID;
    String messageBody;
};

struct SharedChatDelete : public Delete {
    static constexpr std::string_view TAG = "shared_chat_delete";
};

/* automodded message approved
{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_permitted_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"permitted","terms":["cock cock cock penis sex cock"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* automodded message denied
{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_blocked_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"blocked","terms":["boobies"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}

{"subscription":{"id":"0ccc8f11-7e77-40cf-84a9-25ab934c30fb","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"117166826","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.092673205Z","cost":0},"event":{"broadcaster_user_id":"117166826","broadcaster_user_login":"testaccount_420","broadcaster_user_name":"테스트계정420","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"117166826","moderator_user_login":"testaccount_420","moderator_user_name":"테스트계정420","action":"add_blocked_term","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":{"action":"add","list":"blocked","terms":["cock cock cock penis sex cock penis penis cock"],"from_automod":true},"unban_request":null,"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct AutomodTerms {
    static constexpr std::string_view FIELD = "automod_terms";

    // either add or remove
    String action;
    // either blocked or permitted
    String list;

    std::vector<String> terms;
    bool fromAutomod;
};

struct AddBlockedTerm : public AutomodTerms {
    static constexpr std::string_view TAG = "add_blocked_term";
};
struct AddPermittedTerm : public AutomodTerms {
    static constexpr std::string_view TAG = "add_permitted_term";
};
struct RemoveBlockedTerm : public AutomodTerms {
    static constexpr std::string_view TAG = "remove_blocked_term";
};
struct RemovePermittedTerm : public AutomodTerms {
    static constexpr std::string_view TAG = "remove_permitted_term";
};

/* unban request approved
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"approve_unban_request","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":{"is_approved":true,"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","moderator_message":"you have been granted mercy"},"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* unban request denied
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"deny_unban_request","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":{"is_approved":false,"user_id":"975285839","user_login":"selenatormapguy","user_name":"selenatormapguy","moderator_message":""},"warn":null,"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct UnbanRequest {
    static constexpr std::string_view FIELD = "unban_request";

    bool isApproved;

    std::string userID;
    std::string userLogin;
    std::string userName;

    std::string moderatorMessage;
};

struct ApproveUnbanRequest : public UnbanRequest {
    static constexpr std::string_view TAG = "approve_unban_request";
};
struct DenyUnbanRequest : public UnbanRequest {
    static constexpr std::string_view TAG = "deny_unban_request";
};

/* freetext warn from chatterino
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"warn","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","reason":"this is a test warning from chatterino","chat_rules_cited":null},"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

/* warning from web ui
{"subscription":{"id":"4284c08c-402a-43a8-8537-1e75f38f562c","status":"enabled","type":"channel.moderate","version":"2","condition":{"broadcaster_user_id":"11148817","moderator_user_id":"117166826"},"transport":{"method":"websocket","session_id":"AgoQfS3RYz3MSqOophais4HEjxIGY2VsbC1j"},"created_at":"2025-02-01T12:15:13.083098352Z","cost":0},"event":{"broadcaster_user_id":"11148817","broadcaster_user_login":"pajlada","broadcaster_user_name":"pajlada","source_broadcaster_user_id":null,"source_broadcaster_user_login":null,"source_broadcaster_user_name":null,"moderator_user_id":"11148817","moderator_user_login":"pajlada","moderator_user_name":"pajlada","action":"warn","followers":null,"slow":null,"vip":null,"unvip":null,"mod":null,"unmod":null,"ban":null,"unban":null,"timeout":null,"untimeout":null,"raid":null,"unraid":null,"delete":null,"automod_terms":null,"unban_request":null,"warn":{"user_id":"159849156","user_login":"bajlada","user_name":"BajLada","reason":"and a custom reason","chat_rules_cited":["Rule 2"]},"shared_chat_ban":null,"shared_chat_unban":null,"shared_chat_timeout":null,"shared_chat_untimeout":null,"shared_chat_delete":null}}
*/

struct Warn {
    static constexpr std::string_view TAG = "warn";

    String userID;
    String userLogin;
    String userName;
    String reason;

    // TODO: Verify we handle null for this as an empty vector
    std::vector<String> chatRulesCited;
};

struct Clear {
    static constexpr std::string_view TAG = "clear";
};

struct EmoteOnly {
    static constexpr std::string_view TAG = "emoteonly";
};
struct EmoteOnlyOff {
    static constexpr std::string_view TAG = "emoteonlyoff";
};

struct Uniquechat {
    static constexpr std::string_view TAG = "uniquechat";
};
struct UniquechatOff {
    static constexpr std::string_view TAG = "uniquechatoff";
};

struct Subscribers {
    static constexpr std::string_view TAG = "subscribers";
};
struct SubscribersOff {
    static constexpr std::string_view TAG = "subscribersoff";
};

struct Event {
    /// User ID (e.g. 117166826) of the user who's channel the event took place in
    String broadcasterUserID;
    /// User Login (e.g. testaccount_420) of the user who's channel the event took place in
    String broadcasterUserLogin;
    /// User Name (e.g. 테스트계정420) of the user who's channel the event took place in
    String broadcasterUserName;

    /// For Shared Chat events, the user ID (e.g. 117166826) of the user who's channel the event took place in
    std::optional<String> sourceBroadcasterUserID;
    /// For Shared Chat events, the user Login (e.g. testaccount_420) of the user who's channel the event took place in
    std::optional<String> sourceBroadcasterUserLogin;
    /// For Shared Chat events, the user Name (e.g. 테스트계정420) of the user who's channel the event took place in
    std::optional<String> sourceBroadcasterUserName;

    /// User ID (e.g. 117166826) of the user who took the action
    String moderatorUserID;
    /// User Login (e.g. testaccount_420) of the user who took the action
    String moderatorUserLogin;
    /// User Name (e.g. 테스트계정420) of the user who took the action
    String moderatorUserName;

    /// json_tag=action
    std::variant<Ban,                  //
                 Timeout,              //
                 Unban,                //
                 Untimeout,            //
                 Clear,                //
                 EmoteOnly,            //
                 EmoteOnlyOff,         //
                 Followers,            //
                 FollowersOff,         //
                 Uniquechat,           //
                 UniquechatOff,        //
                 Slow,                 //
                 SlowOff,              //
                 Subscribers,          //
                 SubscribersOff,       //
                 Unraid,               //
                 Delete,               //
                 Unvip,                //
                 Vip,                  //
                 Raid,                 //
                 AddBlockedTerm,       //
                 AddPermittedTerm,     //
                 RemoveBlockedTerm,    //
                 RemovePermittedTerm,  //
                 Mod,                  //
                 Unmod,                //
                 ApproveUnbanRequest,  //
                 DenyUnbanRequest,     //
                 Warn,                 //
                 SharedChatBan,        //
                 SharedChatTimeout,    //
                 SharedChatUnban,      //
                 SharedChatUntimeout,  //
                 SharedChatDelete,     //
                 std::string>
        action;

    bool isFromSharedChat() const noexcept;
};

struct Payload {
    subscription::Subscription subscription;

    Event event;
};

#include "twitch-eventsub-ws/payloads/channel-moderate-v2.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_moderate::v2
