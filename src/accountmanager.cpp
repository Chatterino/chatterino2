#include "accountmanager.hpp"
#include "common.hpp"
#include "debug/log.hpp"

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

std::shared_ptr<twitch::TwitchUser> TwitchAccountManager::getCurrent()
{
    if (!this->currentUser) {
        return this->anonymousUser;
    }

    return this->currentUser;
}

std::vector<QString> TwitchAccountManager::getUsernames() const
{
    std::vector<QString> userNames;

    std::lock_guard<std::mutex> lock(this->mutex);

    for (const auto &user : this->users) {
        userNames.push_back(user->getUserName());
    }

    return userNames;
}

std::shared_ptr<twitch::TwitchUser> TwitchAccountManager::findUserByUsername(
    const QString &username) const
{
    std::lock_guard<std::mutex> lock(this->mutex);

    for (const auto &user : this->users) {
        if (username.compare(user->getUserName(), Qt::CaseInsensitive) == 0) {
            return user;
        }
    }

    return nullptr;
}

bool TwitchAccountManager::userExists(const QString &username) const
{
    return this->findUserByUsername(username) != nullptr;
}

bool TwitchAccountManager::addUser(std::shared_ptr<twitch::TwitchUser> user)
{
    if (this->userExists(user->getNickName())) {
        // User already exists in user list
        return false;
    }

    std::lock_guard<std::mutex> lock(this->mutex);

    this->users.push_back(user);

    return true;
}

AccountManager::AccountManager()
{
    this->Twitch.anonymousUser.reset(new twitch::TwitchUser("justinfan64537", "", ""));

    this->Twitch.currentUsername.connect([this](const auto &newValue, auto) {
        QString newUsername(QString::fromStdString(newValue));
        auto user = this->Twitch.findUserByUsername(newUsername);
        if (user) {
            debug::Log("[AccountManager:currentUsernameChanged] User successfully updated to {}",
                       newUsername);
            this->Twitch.currentUser = user;
        } else {
            debug::Log(
                "[AccountManager:currentUsernameChanged] User successfully updated to anonymous");
            this->Twitch.currentUser = this->Twitch.anonymousUser;
        }

        this->Twitch.userChanged.invoke();
    });
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

        auto user =
            std::make_shared<twitch::TwitchUser>(qS(username), qS(oauthToken), qS(clientID));

        this->Twitch.addUser(user);

        printf("Adding user %s(%s)\n", username.c_str(), userID.c_str());
    }

    auto currentUser = this->Twitch.findUserByUsername(
        QString::fromStdString(this->Twitch.currentUsername.getValue()));

    if (currentUser) {
        this->Twitch.currentUser = currentUser;
    } else {
        this->Twitch.currentUser = this->Twitch.anonymousUser;
    }

    this->Twitch.userChanged.invoke();
}

}  // namespace chatterino
