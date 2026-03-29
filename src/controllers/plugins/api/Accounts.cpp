// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/api/Accounts.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "controllers/accounts/AccountController.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "providers/twitch/TwitchAccount.hpp"
#    include "util/WeakPtrHelpers.hpp"

#    include <sol/sol.hpp>

#    include <optional>

namespace {

using namespace chatterino;

class WeakTwitchAccount
{
public:
    WeakTwitchAccount(std::weak_ptr<TwitchAccount> account)
        : weak(std::move(account))
    {
    }

    QString login() const
    {
        return this->shared()->getUserName();
    }

    QString id() const
    {
        return this->shared()->getUserId();
    }

    std::optional<QString> color() const
    {
        auto c = this->shared()->color();
        if (c.isValid())
        {
            return c.name(QColor::HexArgb);
        }
        return std::nullopt;
    }

    bool isAnon() const
    {
        return this->shared()->isAnon();
    }

    bool isValid() const
    {
        return !this->weak.expired();
    }

    bool operator==(const WeakTwitchAccount &other) const
    {
        return weakOwnerEquals(this->weak, other.weak);
    }

private:
    std::shared_ptr<TwitchAccount> shared() const
    {
        auto shared = this->weak.lock();
        if (!shared)
        {
            throw std::runtime_error("Expired account");
        }
        return shared;
    }

    std::weak_ptr<TwitchAccount> weak;
};

}  // namespace

namespace chatterino::lua::api {

void createAccounts(sol::table &c2)
{
    c2.new_usertype<WeakTwitchAccount>(
        // clang-format off
        "TwitchAccount", sol::no_constructor,
        sol::meta_function::to_string, &WeakTwitchAccount::login,
        "is_valid", &WeakTwitchAccount::isValid,
        "login", &WeakTwitchAccount::login,
        "id", &WeakTwitchAccount::id,
        "color", &WeakTwitchAccount::color,
        "is_anon", &WeakTwitchAccount::isAnon
        // clang-format on
    );

    c2.set_function("current_account", [] {
        return WeakTwitchAccount(getApp()->getAccounts()->twitch.getCurrent());
    });
}

}  // namespace chatterino::lua::api

#endif
