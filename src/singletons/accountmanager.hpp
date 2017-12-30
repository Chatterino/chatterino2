#pragma once

#include "twitch/twitchaccountmanager.hpp"

namespace chatterino {

class AccountManager
{
    AccountManager();

public:
    static AccountManager &getInstance();

    void load();

    twitch::TwitchAccountManager Twitch;
};

}  // namespace chatterino
