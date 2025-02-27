#pragma once

#include "twitch-eventsub-ws/string.hpp"

#include <string_view>
#include <variant>
#include <vector>

namespace chatterino::eventsub::lib::chat {

struct Cheermote {
    static constexpr std::string_view TAG = "cheermote";

    String prefix;
    int bits;
    int tier;
};

struct Emote {
    static constexpr std::string_view TAG = "emote";

    String id;
    String emoteSetID;
    // XXX: this isn't included in automod-ish messages and we don't have a
    // need for it right now
    // String ownerID;
};

struct Mention {
    static constexpr std::string_view TAG = "mention";

    String userID;
    String userName;
    String userLogin;
};

struct Text {
    static constexpr std::string_view TAG = "text";
};

struct MessageFragment {
    String text;
    /// json_tag=type
    std::variant<Text, Cheermote, Emote, Mention> inner;
};

struct Message {
    String text;
    std::vector<MessageFragment> fragments;
};

#include "twitch-eventsub-ws/payloads/structured-message.inc"

}  // namespace chatterino::eventsub::lib::chat
