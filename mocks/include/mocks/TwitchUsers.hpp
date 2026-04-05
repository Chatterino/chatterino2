#pragma once

#include "providers/twitch/TwitchUser.hpp"
#include "providers/twitch/TwitchUsers.hpp"

namespace chatterino::mock {

class TwitchUsers : public ITwitchUsers
{
public:
    TwitchUsers() = default;

    std::shared_ptr<TwitchUser> resolveID(const UserId &id) override
    {
        TwitchUser u = {
            .id = id.string,
            .name = "forsen",
            .displayName = "forsen",
            .profilePictureUrl =
                "https://static-cdn.jtvnw.net/jtv_user_pictures/"
                "forsen-profile_image-48b43e1e4f54b5c8-300x300.png",
        };
        return std::make_shared<TwitchUser>(u);
    }
};

}  // namespace chatterino::mock
