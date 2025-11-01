#include "controllers/plugins/api/Accounts.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#include "providers/twitch/TwitchAccount.hpp"
#include "util/WeakPtrHelpers.hpp"

#include <sol/sol.hpp>

namespace {

using namespace chatterino;

class WeakTwitchAccount
{
public:
    WeakTwitchAccount(std::weak_ptr<TwitchAccount> account)
        : weak(std::move(account))
    {
    }

    QString userName() const
    {
        return this->shared()->getUserName();
    }

    QString userID() const
    {
        return this->shared()->getUserId();
    }

    QString color() const
    {
        return this->shared()->color().name(QColor::HexArgb);
    }

    bool isAnon() const
    {
        return this->shared()->isAnon();
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
        "TwitchAccount", sol::no_constructor,  //
        sol::meta_function::to_string,
        &WeakTwitchAccount::userName,                //
        "user_login", &WeakTwitchAccount::userName,  //
        "user_id", &WeakTwitchAccount::userID,       //
        "color", &WeakTwitchAccount::color,          //
        "is_anon", &WeakTwitchAccount::isAnon        //
    );

    c2.set_function("current_account", [] {
        return WeakTwitchAccount(getApp()->getAccounts()->twitch.getCurrent());
    });
}

}  // namespace chatterino::lua::api
