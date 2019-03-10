#pragma once

#include "common/SignalVector.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/ChatterinoSetting.hpp"
#include "util/SharedPtrElementLess.hpp"

#include <mutex>
#include <vector>

//
// Warning: This class is not supposed to be created directly.
// 			Get yourself an instance from our friends over at
// AccountManager.hpp
//

namespace chatterino
{
    class TwitchAccount;
    class AccountController;

    class TwitchAccountManager
    {
        TwitchAccountManager();

    public:
        struct UserData
        {
            QString username;
            QString userID;
            QString clientID;
            QString oauthToken;
        };

        // Returns the current twitchUsers, or the anonymous user if we're not
        // currently logged in
        std::shared_ptr<TwitchAccount> getCurrent();

        std::vector<QString> getUsernames() const;

        std::shared_ptr<TwitchAccount> findUserByUsername(
            const QString& username) const;
        bool userExists(const QString& username) const;

        void reloadUsers();
        void load();

        bool isLoggedIn() const;

        pajlada::Settings::Setting<QString> currentUsername{
            "/accounts/current", ""};
        pajlada::Signals::NoArgSignal currentUserChanged;
        pajlada::Signals::NoArgSignal userListUpdated;

        SortedSignalVector<std::shared_ptr<TwitchAccount>,
            SharedPtrElementLess<TwitchAccount>>
            accounts;

    private:
        enum class AddUserResponse {
            UserAlreadyExists,
            UserValuesUpdated,
            UserAdded,
        };
        AddUserResponse addUser(const UserData& data);
        bool removeUser(TwitchAccount* account);

        std::shared_ptr<TwitchAccount> currentUser_;

        std::shared_ptr<TwitchAccount> anonymousUser_;
        mutable std::mutex mutex_;

        friend class AccountController;
    };

}  // namespace chatterino
