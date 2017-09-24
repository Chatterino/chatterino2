#pragma once

#include "twitch/twitchuser.hpp"

#include <pajlada/settings/setting.hpp>

#include <mutex>
#include <vector>

namespace chatterino {

class AccountManager
{
public:
    static AccountManager &getInstance()
    {
        static AccountManager instance;
        return instance;
    }

    void load();

    twitch::TwitchUser &getTwitchAnon();

    // Returns first user from twitchUsers, or twitchAnonymousUser if twitchUsers is empty
    twitch::TwitchUser &getTwitchUser();

    // Return a copy of the current available twitch users
    std::vector<twitch::TwitchUser> getTwitchUsers();

    // Remove twitch user with the given username
    bool removeTwitchUser(const QString &userName);

    void setCurrentTwitchUser(const QString &username);

    // Add twitch user to the list of available twitch users
    void addTwitchUser(const twitch::TwitchUser &user);

private:
    AccountManager();

    pajlada::Settings::Setting<std::string> currentUser;

    twitch::TwitchUser twitchAnonymousUser;
    std::vector<twitch::TwitchUser> twitchUsers;
    std::mutex twitchUsersMutex;
};

}  // namespace chatterino
