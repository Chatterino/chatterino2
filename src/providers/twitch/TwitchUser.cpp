#include "providers/twitch/TwitchUser.hpp"

#include "debug/AssertInGuiThread.hpp"
#include "providers/twitch/api/Helix.hpp"

namespace chatterino {

void TwitchUser::fromHelixBlock(const HelixBlock &ignore)
{
    this->id = ignore.userId;
    this->name = ignore.userName;
    this->displayName = ignore.displayName;
}

void TwitchUser::update(const HelixUser &user) const
{
    assertInGuiThread();
    assert(this->id == user.id);
    this->name = user.login;
    this->displayName = user.displayName;
}

}  // namespace chatterino
