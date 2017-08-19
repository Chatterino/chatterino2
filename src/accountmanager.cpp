#include "accountmanager.hpp"
#include "common.hpp"

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
}

void AccountManager::load()
{
    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/accounts");

    for (const auto &uid : keys) {
        if (uid == "current") {
            continue;
        }

        std::string username =
            pajlada::Settings::Setting<std::string>::get("/accounts/" + uid + "/username");
        std::string userID =
            pajlada::Settings::Setting<std::string>::get("/accounts/" + uid + "/userID");
        std::string clientID =
            pajlada::Settings::Setting<std::string>::get("/accounts/" + uid + "/clientID");
        std::string oauthToken =
            pajlada::Settings::Setting<std::string>::get("/accounts/" + uid + "/oauthToken");

        if (username.empty() || userID.empty() || clientID.empty() || oauthToken.empty()) {
            continue;
        }

        twitch::TwitchUser user(qS(username), qS(oauthToken), qS(clientID));

        this->addTwitchUser(user);

        printf("Adding user %s(%s)\n", username.c_str(), userID.c_str());
    }
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
