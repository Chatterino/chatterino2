#include "accountmanager.h"

namespace chatterino {

namespace {

inline QString getEnvString(const char *target)
{
    char *val = std::getenv(target);
    if (val == nullptr) {
        return QString();
    }

    return QString(val);
}

}  // namespace

AccountManager::AccountManager()
    : _twitchAnon("justinfan64537", "", "")
    , _twitchUsers()
    , _twitchUsersMutex()
{
    QString envUsername = getEnvString("CHATTERINO2_USERNAME");
    QString envOauthToken = getEnvString("CHATTERINO2_OAUTH");

    if (!envUsername.isEmpty() && !envOauthToken.isEmpty()) {
        this->addTwitchUser(twitch::TwitchUser(envUsername, envOauthToken, ""));
    }
}

twitch::TwitchUser &AccountManager::getTwitchAnon()
{
    return _twitchAnon;
}

twitch::TwitchUser &AccountManager::getTwitchUser()
{
    std::lock_guard<std::mutex> lock(_twitchUsersMutex);

    if (_twitchUsers.size() == 0) {
        return _twitchAnon;
    }

    return _twitchUsers.front();
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
