#pragma once

#include <QObject>

#include "controllers/accounts/Account.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "util/SharedPtrElementLess.hpp"
#include "util/SignalVector2.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

class AccountModel;

class AccountController
{
public:
    AccountController();

    AccountModel *createModel(QObject *parent);

    void load();

    providers::twitch::TwitchAccountManager twitch;

private:
    util::SortedSignalVector<std::shared_ptr<Account>, util::SharedPtrElementLess<Account>>
        accounts;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
