#pragma once

#include "twitch/twitchuser.hpp"

#include <pajlada/settings/setting.hpp>

#include <mutex>
#include <vector>

namespace chatterino {

class AccountManager;

class TwitchAccountManager
{
public:
    // Returns the current twitchUsers, or the anonymous user if we're not currently logged in
    std::shared_ptr<twitch::TwitchUser> getCurrent();

    std::vector<QString> getUsernames() const;

    std::shared_ptr<twitch::TwitchUser> findUserByUsername(const QString &username) const;
    bool userExists(const QString &username) const;

    pajlada::Settings::Setting<std::string> currentUsername = {"/accounts/current", ""};
    pajlada::Signals::NoArgSignal userChanged;

private:
    bool addUser(std::shared_ptr<twitch::TwitchUser> user);

    std::shared_ptr<twitch::TwitchUser> currentUser;

    std::shared_ptr<twitch::TwitchUser> anonymousUser;
    std::vector<std::shared_ptr<twitch::TwitchUser>> users;
    mutable std::mutex mutex;

    friend class AccountManager;
};

class AccountManager
{
public:
    static AccountManager &getInstance()
    {
        static AccountManager instance;
        return instance;
    }

    void load();

    TwitchAccountManager Twitch;

private:
    AccountManager();
};

}  // namespace chatterino
