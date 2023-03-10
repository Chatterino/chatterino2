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

}  // namespace chatterino
