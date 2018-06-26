#pragma once

#include <QObject>

#include "controllers/accounts/Account.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "util/SharedPtrElementLess.hpp"
#include "common/SignalVector2.hpp"

namespace chatterino {

class AccountModel;

class AccountController
{
public:
    AccountController();

    AccountModel *createModel(QObject *parent);

    void load();

    TwitchAccountManager twitch;

private:
    SortedSignalVector<std::shared_ptr<Account>, SharedPtrElementLess<Account>>
        accounts;
};

}  // namespace chatterino
