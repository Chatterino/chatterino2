#pragma once

#include "providers/twitch/twitchaccountmanager.hpp"

namespace chatterino {
namespace singletons {

class AccountManager
{
    AccountManager();

public:
    static AccountManager &getInstance();

    void load();

    providers::twitch::TwitchAccountManager Twitch;
};

}  // namespace singletons
}  // namespace chatterino
