#pragma once

#include "common/Aliases.hpp"

#include <memory>

namespace chatterino {

struct TwitchUser;
class TwitchChannel;

class ITwitchUsers
{
public:
    ITwitchUsers() = default;
    virtual ~ITwitchUsers() = default;
    ITwitchUsers(const ITwitchUsers &) = delete;
    ITwitchUsers(ITwitchUsers &&) = delete;
    ITwitchUsers &operator=(const ITwitchUsers &) = delete;
    ITwitchUsers &operator=(ITwitchUsers &&) = delete;

    /// @brief Resolve a TwitchUser by their ID
    ///
    /// Users are cached. If the user wasn't resolved yet, a request will be
    /// scheduled. The returned shared pointer must only be used on the GUI
    /// thread as it will be updated from there.
    ///
    /// @returns A shared reference to the TwitchUser. The `name` and
    ///          `displayName` might be empty if the user wasn't resolved yet or
    ///          they don't exist.
    virtual std::shared_ptr<TwitchUser> resolveID(const UserId &id) = 0;
};

class TwitchUsersPrivate;
class TwitchUsers : public ITwitchUsers
{
public:
    TwitchUsers();
    ~TwitchUsers() override;
    TwitchUsers(const TwitchUsers &) = delete;
    TwitchUsers(TwitchUsers &&) = delete;
    TwitchUsers &operator=(const TwitchUsers &) = delete;
    TwitchUsers &operator=(TwitchUsers &&) = delete;

    /// @see ITwitchUsers::resolveID()
    std::shared_ptr<TwitchUser> resolveID(const UserId &id) override;

private:
    // Using a shared_ptr to pass to network callbacks
    std::shared_ptr<TwitchUsersPrivate> private_;

    friend TwitchUsersPrivate;
};

}  // namespace chatterino
