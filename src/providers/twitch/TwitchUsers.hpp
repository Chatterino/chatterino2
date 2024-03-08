#pragma once

#include "common/Aliases.hpp"

#include <QStringList>

#include <memory>
#include <optional>

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

    std::shared_ptr<TwitchUser> resolveID(const UserId &id) override;

private:
    // Using a shared_ptr to pass to network callbacks
    std::shared_ptr<TwitchUsersPrivate> private_;

    friend TwitchUsersPrivate;
};

}  // namespace chatterino
