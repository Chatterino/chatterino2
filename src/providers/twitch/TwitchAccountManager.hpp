#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "util/Expected.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <boost/signals2.hpp>
#include <QString>
#include <QTimer>

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
struct TwitchAccountData;
class AccountController;

extern const std::vector<QStringView> DEVICE_AUTH_SCOPES;
extern const QString DEVICE_AUTH_CLIENT_ID;

class TwitchAccountManager
{
    TwitchAccountManager();

public:
    struct UserData;
    using UserCallback =
        std::function<void(const std::shared_ptr<TwitchAccount> &)>;

    // Returns the current twitchUsers, or the anonymous user if we're not
    // currently logged in
    std::shared_ptr<TwitchAccount> getCurrent();

    void requestCurrent(UserCallback cb);
    void requestCurrentChecked(UserCallback cb);

    void recheckRefresher();

    std::vector<QString> getUsernames() const;

    std::shared_ptr<TwitchAccount> findUserByUsername(
        const QString &username) const;
    bool userExists(const QString &username) const;

    void reloadUsers();
    void load();

    bool isLoggedIn() const;

    pajlada::Settings::Setting<QString> currentUsername{"/accounts/current",
                                                        ""};
    // pajlada::Signals::NoArgSignal currentUserChanged;
    boost::signals2::signal<void()> currentUserChanged;
    pajlada::Signals::NoArgSignal userListUpdated;

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
    AddUserResponse addUser(const TwitchAccountData &data);
    bool removeUser(TwitchAccount *account);
    void refreshAccounts(bool emitChanged);

    bool isRefreshing_ = false;
    std::vector<UserCallback> pendingUserConsumers_;
    QTimer refreshTask_;

    std::shared_ptr<TwitchAccount> currentUser_;

    std::shared_ptr<TwitchAccount> anonymousUser_;
    mutable std::mutex mutex_;

    friend class AccountController;
};

}  // namespace chatterino
