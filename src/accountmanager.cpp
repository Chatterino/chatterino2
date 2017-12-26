#include "accountmanager.hpp"
#include "common.hpp"
#include "const.hpp"
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

TwitchAccountManager::TwitchAccountManager()
{
    this->anonymousUser.reset(new twitch::TwitchUser(twitch::ANONYMOUS_USERNAME, "", ""));

    this->currentUsername.connect([this](const auto &newValue, auto) {
        QString newUsername(QString::fromStdString(newValue));
        auto user = this->findUserByUsername(newUsername);
        if (user) {
            debug::Log("[AccountManager:currentUsernameChanged] User successfully updated to {}",
                       newUsername);
            this->currentUser = user;
        } else {
            debug::Log(
                "[AccountManager:currentUsernameChanged] User successfully updated to anonymous");
            this->currentUser = this->anonymousUser;
        }

        this->userChanged.invoke();
    });
}

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

void TwitchAccountManager::reloadUsers()
{
    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/accounts");

    UserData userData;

    bool listUpdated = false;

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

        userData.username = qS(username);
        userData.userID = qS(userID);
        userData.clientID = qS(clientID);
        userData.oauthToken = qS(oauthToken);

        switch (this->addUser(userData)) {
            case AddUserResponse::UserAlreadyExists: {
                debug::Log("User {} already exists", userData.username);
                // Do nothing
            } break;
            case AddUserResponse::UserValuesUpdated: {
                debug::Log("User {} already exists, and values updated!", userData.username);
                if (userData.username == this->getCurrent()->getNickName()) {
                    debug::Log("It was the current user, so we need to reconnect stuff!");
                    this->userChanged.invoke();
                }
            } break;
            case AddUserResponse::UserAdded: {
                debug::Log("Added user {}", userData.username);
                listUpdated = true;
            } break;
        }
    }

    if (listUpdated) {
        this->userListUpdated.invoke();
    }
}

bool TwitchAccountManager::removeUser(const QString &username)
{
    if (!this->userExists(username)) {
        return false;
    }

    this->mutex.lock();
    this->users.erase(std::remove_if(this->users.begin(), this->users.end(), [username](auto user) {
        if (user->getNickName() == username) {
            std::string userID(user->getUserId().toStdString());
            assert(!userID.empty());
            pajlada::Settings::SettingManager::removeSetting("/accounts/uid" + userID);
            return true;
        }
        return false;
    }));
    this->mutex.unlock();

    if (username == qS(this->currentUsername.getValue())) {
        // The user that was removed is the current user, log into the anonymous user
        this->currentUsername = "";
    }

    this->userListUpdated.invoke();

    return true;
}

TwitchAccountManager::AddUserResponse TwitchAccountManager::addUser(
    const TwitchAccountManager::UserData &userData)
{
    auto previousUser = this->findUserByUsername(userData.username);
    if (previousUser) {
        bool userUpdated = false;

        if (previousUser->setOAuthClient(userData.clientID)) {
            userUpdated = true;
        }

        if (previousUser->setOAuthToken(userData.oauthToken)) {
            userUpdated = true;
        }

        if (userUpdated) {
            return AddUserResponse::UserValuesUpdated;
        } else {
            return AddUserResponse::UserAlreadyExists;
        }
    }

    auto newUser = std::make_shared<twitch::TwitchUser>(userData.username, userData.oauthToken,
                                                        userData.clientID);

    // Set users User ID without the uid prefix
    newUser->setUserId(userData.userID);

    std::lock_guard<std::mutex> lock(this->mutex);

    this->users.push_back(newUser);

    return AddUserResponse::UserAdded;
}

AccountManager::AccountManager()
{
}

void AccountManager::load()
{
    this->Twitch.reloadUsers();

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
