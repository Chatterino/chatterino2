#pragma once

#include "twitch/twitchaccountmanager.hpp"

namespace chatterino {
namespace singletons {

class AccountManager
{
    AccountManager();

public:
    static AccountManager &getInstance();

    void load();

    twitch::TwitchAccountManager Twitch;
};

}  // namespace singletons
}  // namespace chatterino
