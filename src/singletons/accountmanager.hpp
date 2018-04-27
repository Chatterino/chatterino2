#pragma once

#include "providers/twitch/twitchaccountmanager.hpp"

namespace chatterino {
namespace singletons {

class AccountManager
{
    AccountManager() = default;
    friend class Application;

public:
    ~AccountManager() = delete;

    void load();

    providers::twitch::TwitchAccountManager Twitch;
};

}  // namespace singletons
}  // namespace chatterino
