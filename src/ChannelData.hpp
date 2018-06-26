#pragma once

#include "LockedObject.hpp"
#include "messages/Image.hpp"

namespace chatterino {

class ChannelData
{
public:
    ChannelData() = default;

    LockedObject<std::string> username;
    LockedObject<std::string> id;

private:
    // std::map<std::string, BadgeData> subscriptionBadges;
};

}  // namespace chatterino
