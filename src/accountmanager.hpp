#pragma once

#include "twitch/twitchuser.hpp"

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

    twitch::TwitchUser &getTwitchAnon();

    // Returns first user from _twitchUsers, or _twitchAnon if _twitchUsers is empty
    twitch::TwitchUser &getTwitchUser();

    std::vector<twitch::TwitchUser> getTwitchUsers();
    bool removeTwitchUser(const QString &userName);
    void addTwitchUser(const twitch::TwitchUser &user);

private:
    AccountManager();

    twitch::TwitchUser _twitchAnon;
    std::vector<twitch::TwitchUser> _twitchUsers;
    std::mutex _twitchUsersMutex;
};

}  // namespace chatterino
