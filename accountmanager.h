#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "twitch/twitchaccount.h"

#include <mutex>
#include <vector>

namespace chatterino {

class AccountManager
{
public:
    static AccountManager &getInstance()
    {
        return instance;
    }

    twitch::TwitchUser &getTwitchAnon();

    std::vector<twitch::TwitchUser> getTwitchUsers();
    bool removeTwitchUser(const QString &userName);
    void addTwitchUser(const twitch::TwitchUser &user);

private:
    static AccountManager instance;

    AccountManager();

    twitch::TwitchUser _twitchAnon;
    std::vector<twitch::TwitchUser> _twitchUsers;
    std::mutex _twitchUsersMutex;
};
}  // namespace chatterino

#endif  // ACCOUNTMANAGER_H
