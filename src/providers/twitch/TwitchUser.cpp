#include "providers/twitch/TwitchUser.hpp"

#include "providers/twitch/api/Helix.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

void TwitchUser::fromHelixBlock(const HelixBlock &ignore)
{
    this->id = ignore.userId;
    this->name = ignore.userName;
    this->displayName = ignore.displayName;
}

void TwitchUser::update(const HelixUser &user) const
{
    assert(this->id == user.id);
    this->name = user.login;
    this->displayName = user.displayName;
}

}  // namespace chatterino
