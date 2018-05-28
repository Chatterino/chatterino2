#include "providers/twitch/twitchaccountmanager.hpp"

#include "common.hpp"
#include "const.hpp"
#include "debug/log.hpp"

namespace chatterino {
namespace providers {
namespace twitch {

TwitchAccountManager::TwitchAccountManager()
    : anonymousUser(new TwitchAccount(ANONYMOUS_USERNAME, "", "", ""))
{
    this->currentUserChanged.connect([this] {
        auto currentUser = this->getCurrent();
        currentUser->loadIgnores();
    });
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::getCurrent()
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

    for (const auto &user : this->accounts.getVector()) {
        userNames.push_back(user->getUserName());
    }

    return userNames;
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::findUserByUsername(
    const QString &username) const
{
    std::lock_guard<std::mutex> lock(this->mutex);

    for (const auto &user : this->accounts.getVector()) {
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
                if (userData.username == this->getCurrent()->getUserName()) {
                    debug::Log("It was the current user, so we need to reconnect stuff!");
                    this->currentUserChanged.invoke();
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

void TwitchAccountManager::load()
{
    this->reloadUsers();

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

        this->currentUserChanged.invoke();
    });
}

bool TwitchAccountManager::removeUser(const QString &username)
{
    if (!this->userExists(username)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> guard(this->mutex);

        const auto &accs = this->accounts.getVector();

        while (true) {
            auto it = std::find_if(accs.begin(), accs.end(),
                                   [&](const auto &acc) { return acc->getUserName() == username; });

            if (it == accs.end()) {
                break;
            }

            std::string userID(it->get()->getUserId().toStdString());
            if (!userID.empty()) {
                pajlada::Settings::SettingManager::removeSetting("/accounts/uid" + userID);
            }

            this->accounts.removeItem(int(it - accs.begin()));
        }
    }

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

    auto newUser = std::make_shared<TwitchAccount>(userData.username, userData.oauthToken,
                                                   userData.clientID, userData.userID);

    std::lock_guard<std::mutex> lock(this->mutex);

    this->accounts.insertItem(newUser);

    return AddUserResponse::UserAdded;
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
