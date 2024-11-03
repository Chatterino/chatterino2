#pragma once

#include "providers/twitch/TwitchUser.hpp"
#include "providers/twitch/TwitchUsers.hpp"

namespace chatterino::mock {

class TwitchUsers : public ITwitchUsers
{
public:
    TwitchUsers() = default;

    std::shared_ptr<TwitchUser> resolveID(const UserId &id)
    {
        TwitchUser u = {
            .id = id.string,
            .name = {},
            .displayName = {},
        };
        return std::make_shared<TwitchUser>(u);
    }
};

}  // namespace chatterino::mock
