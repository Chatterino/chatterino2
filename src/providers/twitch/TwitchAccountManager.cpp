#include "providers/twitch/TwitchAccountManager.hpp"

#include "common/Common.hpp"
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

    this->accounts.itemRemoved.connect([this](const auto &acc) {  //
        this->removeUser(acc.item.get());
    });
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::getCurrent()
{
    if (!this->currentUser_)
    {
        return this->anonymousUser_;
    }

    return this->currentUser_;
}

std::vector<QString> TwitchAccountManager::getUsernames() const
{
    std::vector<QString> userNames;

    std::lock_guard<std::mutex> lock(this->mutex_);

    for (const auto &user : this->accounts)
    {
        userNames.push_back(user->getUserName());
    }

    return userNames;
}

std::shared_ptr<TwitchAccount> TwitchAccountManager::findUserByUsername(
    const QString &username) const
{
    std::lock_guard<std::mutex> lock(this->mutex_);

    for (const auto &user : this->accounts)
    {
        if (username.compare(user->getUserName(), Qt::CaseInsensitive) == 0)
        {
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

    for (const auto &uid : keys)
    {
        if (uid == "current")
        {
            continue;
        }

        auto username = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/username");
        auto userID = pajlada::Settings::Setting<QString>::get("/accounts/" +
                                                               uid + "/userID");
        auto clientID = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/clientID");
        auto oauthToken = pajlada::Settings::Setting<QString>::get(
            "/accounts/" + uid + "/oauthToken");

        if (username.isEmpty() || userID.isEmpty() || clientID.isEmpty() ||
            oauthToken.isEmpty())
        {
            continue;
        }

        userData.username = username.trimmed();
        userData.userID = userID.trimmed();
        userData.clientID = clientID.trimmed();
        userData.oauthToken = oauthToken.trimmed();

        switch (this->addUser(userData))
        {
            case AddUserResponse::UserAlreadyExists: {
                qDebug() << "User" << userData.username << "already exists";
                // Do nothing
            }
            break;
            case AddUserResponse::UserValuesUpdated: {
                qDebug() << "User" << userData.username
                         << "already exists, and values updated!";
                if (userData.username == this->getCurrent()->getUserName())
                {
                    qDebug() << "It was the current user, so we need to "
                                "reconnect stuff!";
                    this->currentUserChanged.invoke();
                }
            }
            break;
            case AddUserResponse::UserAdded: {
                qDebug() << "Added user" << userData.username;
                listUpdated = true;
            }
            break;
        }
    }

    if (listUpdated)
    {
        this->userListUpdated.invoke();
    }
}

void TwitchAccountManager::load()
{
    this->reloadUsers();

    this->currentUsername.connect([this](const QString &newUsername) {
        auto user = this->findUserByUsername(newUsername);
        if (user)
        {
            qDebug() << "Twitch user updated to" << newUsername;
            this->currentUser_ = user;
        }
        else
        {
            qDebug() << "Twitch user updated to anonymous";
            this->currentUser_ = this->anonymousUser_;
        }

        this->currentUserChanged.invoke();
    });
}

bool TwitchAccountManager::isLoggedIn() const
{
    if (!this->currentUser_)
    {
        return false;
    }

    // Once `TwitchAccount` class has a way to check, we should also return
    // false if the credentials are incorrect
    return !this->currentUser_->isAnon();
}

bool TwitchAccountManager::removeUser(TwitchAccount *account)
{
    static const QString accountFormat("/accounts/uid%1");

    auto userID(account->getUserId());
    if (!userID.isEmpty())
    {
        pajlada::Settings::SettingManager::removeSetting(
            accountFormat.arg(userID).toStdString());
    }

    if (account->getUserName() == this->currentUsername)
    {
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
    if (previousUser)
    {
        bool userUpdated = false;

        if (previousUser->setOAuthClient(userData.clientID))
        {
            userUpdated = true;
        }

        if (previousUser->setOAuthToken(userData.oauthToken))
        {
            userUpdated = true;
        }

        if (userUpdated)
        {
            return AddUserResponse::UserValuesUpdated;
        }
        else
        {
            return AddUserResponse::UserAlreadyExists;
        }
    }

    auto newUser =
        std::make_shared<TwitchAccount>(userData.username, userData.oauthToken,
                                        userData.clientID, userData.userID);

    //    std::lock_guard<std::mutex> lock(this->mutex);

    this->accounts.insert(newUser);

    return AddUserResponse::UserAdded;
}

}  // namespace chatterino
