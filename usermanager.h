#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "twitch/twitchuser.h"

namespace chatterino {

class AccountManager
{
public:
    static AccountManager &getInstance()
    {
        return instance;
    }

    twitch::TwitchUser &getAnon();

private:
    static AccountManager instance;

    AccountManager();

    twitch::TwitchUser _anon;
};
}

#endif  // ACCOUNTMANAGER_H
