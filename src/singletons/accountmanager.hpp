#pragma once

#include "providers/twitch/twitchaccountmanager.hpp"

namespace chatterino {
namespace singletons {

class AccountManager
{
public:
    AccountManager() = default;

    ~AccountManager() = delete;

    void load();

    providers::twitch::TwitchAccountManager Twitch;
};

}  // namespace singletons
}  // namespace chatterino
