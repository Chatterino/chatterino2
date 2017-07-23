#include "accountmanager.hpp"

#include <pajlada/settings/setting.hpp>

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
    : twitchAnonymousUser("justinfan64537", "", "")
{
    QString envUsername = getEnvString("CHATTERINO2_USERNAME");
    QString envOauthToken = getEnvString("CHATTERINO2_OAUTH");

    if (!envUsername.isEmpty() && !envOauthToken.isEmpty()) {
        this->addTwitchUser(twitch::TwitchUser(envUsername, envOauthToken, ""));
    }

    pajlada::Settings::Setting<std::string>::set(
        "/accounts/current/roomID", "11148817", pajlada::Settings::SettingOption::DoNotWriteToJSON);
}

twitch::TwitchUser &AccountManager::getTwitchAnon()
{
    return this->twitchAnonymousUser;
}

twitch::TwitchUser &AccountManager::getTwitchUser()
{
    std::lock_guard<std::mutex> lock(this->twitchUsersMutex);

    if (this->twitchUsers.size() == 0) {
        return this->getTwitchAnon();
    }

    return this->twitchUsers.front();
}

std::vector<twitch::TwitchUser> AccountManager::getTwitchUsers()
{
    std::lock_guard<std::mutex> lock(this->twitchUsersMutex);

    return std::vector<twitch::TwitchUser>(this->twitchUsers);
}

bool AccountManager::removeTwitchUser(const QString &userName)
{
    std::lock_guard<std::mutex> lock(this->twitchUsersMutex);

    for (auto it = this->twitchUsers.begin(); it != this->twitchUsers.end(); it++) {
        if ((*it).getUserName() == userName) {
            this->twitchUsers.erase(it);
            return true;
        }
    }

    return false;
}

void AccountManager::addTwitchUser(const twitch::TwitchUser &user)
{
    std::lock_guard<std::mutex> lock(this->twitchUsersMutex);

    this->twitchUsers.push_back(user);
}

}  // namespace chatterino
