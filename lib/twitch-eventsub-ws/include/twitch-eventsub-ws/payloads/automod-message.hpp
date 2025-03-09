#pragma once

#include "twitch-eventsub-ws/string.hpp"

#include <boost/json.hpp>

#include <string>
#include <vector>

namespace chatterino::eventsub::lib::automod {

struct Boundary {
    int startPos;
    int endPos;
};

struct AutomodReason {
    static constexpr std::string_view TAG = "automod";

    String category;
    int level;
    std::vector<Boundary> boundaries;
};

struct FoundTerm {
    std::string termID;
    Boundary boundary;
    std::string ownerBroadcasterUserID;
    std::string ownerBroadcasterUserLogin;
    std::string ownerBroadcasterUserName;
};

struct BlockedTermReason {
    static constexpr std::string_view TAG = "blocked_term";

    std::vector<FoundTerm> termsFound;
};

#include "twitch-eventsub-ws/payloads/automod-message.inc"

}  // namespace chatterino::eventsub::lib::automod
