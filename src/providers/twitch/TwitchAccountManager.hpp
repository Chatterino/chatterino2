// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "util/Expected.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <boost/signals2.hpp>
#include <QString>

#include <memory>
#include <mutex>
#include <vector>

//
// Warning: This class is not supposed to be created directly.
// 			Get yourself an instance from our friends over at
// AccountManager.hpp
//

namespace chatterino {

class TwitchAccount;
class AccountController;

extern const std::vector<QStringView> AUTH_SCOPES;

class TwitchAccountManager
{
    TwitchAccountManager();

public:
    struct UserData {
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
        const QString &username) const;
    bool userExists(const QString &username) const;

    void reloadUsers();
    void load();

    bool isLoggedIn() const;

    pajlada::Settings::Setting<QString> currentUsername{"/accounts/current",
                                                        ""};

    /// This signal fires after we've figured out what the new account is, but before
    /// any updates to Helix have been made.
    ///
    /// Useful for scenarios where you have to call Helix using the previous account.
    pajlada::Signals::Signal<std::shared_ptr<TwitchAccount>,
                             std::shared_ptr<TwitchAccount>>
        currentUserAboutToChange;

    boost::signals2::signal<void()> currentUserChanged;
    pajlada::Signals::NoArgSignal userListUpdated;
    pajlada::Signals::NoArgSignal currentUserNameChanged;

    SignalVector<std::shared_ptr<TwitchAccount>> accounts;

    /// The signal is invoked with (caller, error) where caller is the argument
    /// passed to reloadEmotes() and error.
    pajlada::Signals::Signal<void *, ExpectedStr<void>> emotesReloaded;

private:
    enum class AddUserResponse {
        UserAlreadyExists,
        UserValuesUpdated,
        UserAdded,
    };
    AddUserResponse addUser(const UserData &data);
    bool removeUser(TwitchAccount *account);

    std::shared_ptr<TwitchAccount> currentUser_;

    std::shared_ptr<TwitchAccount> anonymousUser_;
    mutable std::mutex mutex_;

    friend class AccountController;
};

}  // namespace chatterino
