#include "providers/twitch/TwitchAccountManager.hpp"

#include "common/Common.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchCommon.hpp"

namespace chatterino {

TwitchAccountManager::TwitchAccountManager()
    : anonymousUser_(new TwitchAccount(ANONYMOUS_USERNAME, "", "", ""))
{
    this->currentUserChanged.connect([this] {
        auto currentUser = this->getCurrent();
        currentUser->loadIgnores();
    });

    this->accounts.itemRemoved.connect(
        [this](const auto &acc) { this->removeUser(acc.item.get()); });
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::getCurrent()
{
    if (!this->currentUser_) {
        return this->anonymousUser_;
    }

    return this->currentUser_;
}

std::vector<QString> TwitchAccountManager::getUsernames() const
{
    std::vector<QString> userNames;

    std::lock_guard<std::mutex> lock(this->mutex_);

    for (const auto &user : this->accounts.getVector()) {
        userNames.push_back(user->getUserName());
    }

    return userNames;
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::findUserByUsername(
    const QString &username) const
{
    std::lock_guard<std::mutex> lock(this->mutex_);

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

        std::string username = pajlada::Settings::Setting<std::string>::get(
            "/accounts/" + uid + "/username");
        std::string userID = pajlada::Settings::Setting<std::string>::get(
            "/accounts/" + uid + "/userID");
        std::string clientID = pajlada::Settings::Setting<std::string>::get(
            "/accounts/" + uid + "/clientID");
        std::string oauthToken = pajlada::Settings::Setting<std::string>::get(
            "/accounts/" + uid + "/oauthToken");

        if (username.empty() || userID.empty() || clientID.empty() ||
            oauthToken.empty()) {
            continue;
        }

        userData.username = qS(username).trimmed();
        userData.userID = qS(userID).trimmed();
        userData.clientID = qS(clientID).trimmed();
        userData.oauthToken = qS(oauthToken).trimmed();

        switch (this->addUser(userData)) {
            case AddUserResponse::UserAlreadyExists: {
                log("User {} already exists", userData.username);
                // Do nothing
            } break;
            case AddUserResponse::UserValuesUpdated: {
                log("User {} already exists, and values updated!",
                    userData.username);
                if (userData.username == this->getCurrent()->getUserName()) {
                    log("It was the current user, so we need to reconnect "
                        "stuff!");
                    this->currentUserChanged.invoke();
                }
            } break;
            case AddUserResponse::UserAdded: {
                log("Added user {}", userData.username);
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
            log("[AccountManager:currentUsernameChanged] User successfully "
                "updated to {}",
                newUsername);
            this->currentUser_ = user;
        } else {
            log("[AccountManager:currentUsernameChanged] User successfully "
                "updated to anonymous");
            this->currentUser_ = this->anonymousUser_;
        }

        this->currentUserChanged.invoke();
    });
}

bool TwitchAccountManager::isLoggedIn() const
{
    if (!this->currentUser_) {
        return false;
    }

    // Once `TwitchAccount` class has a way to check, we should also return
    // false if the credentials are incorrect
    return !this->currentUser_->isAnon();
}

bool TwitchAccountManager::removeUser(TwitchAccount *account)
{
    const auto &accs = this->accounts.getVector();

    std::string userID(account->getUserId().toStdString());
    if (!userID.empty()) {
        pajlada::Settings::SettingManager::removeSetting("/accounts/uid" +
                                                         userID);
    }

    if (account->getUserName() == qS(this->currentUsername.getValue())) {
        // The user that was removed is the current user, log into the anonymous
        // user
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

    auto newUser =
        std::make_shared<TwitchAccount>(userData.username, userData.oauthToken,
                                        userData.clientID, userData.userID);

    //    std::lock_guard<std::mutex> lock(this->mutex);

    this->accounts.insertItem(newUser);

    return AddUserResponse::UserAdded;
}

}  // namespace chatterino
