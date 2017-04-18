#include "accountmanager.h"

namespace chatterino {

AccountManager AccountManager::instance;

AccountManager::AccountManager()
    : _twitchAnon("justinfan64537", "", "")
    , _twitchUsers()
    , _twitchUsersMutex()
{
}

twitch::TwitchUser &AccountManager::getTwitchAnon()
{
    return _twitchAnon;
}

std::vector<twitch::TwitchUser> AccountManager::getTwitchUsers()
{
    return std::vector<twitch::TwitchUser>(_twitchUsers);
}

bool AccountManager::removeTwitchUser(const QString &userName)
{
    std::lock_guard<std::mutex> lock(_twitchUsersMutex);

    for (auto it = _twitchUsers.begin(); it != _twitchUsers.end(); it++) {
        if ((*it).getUserName() == userName) {
            _twitchUsers.erase(it);
            return true;
        }
    }

    return false;
}

void AccountManager::addTwitchUser(const twitch::TwitchUser &user)
{
    std::lock_guard<std::mutex> lock(_twitchUsersMutex);

    _twitchUsers.push_back(user);
}
}  // namespace chatterino
