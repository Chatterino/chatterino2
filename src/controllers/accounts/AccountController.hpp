#pragma once

#include <QObject>

#include "common/SignalVector.hpp"
#include "controllers/accounts/Account.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "util/SharedPtrElementLess.hpp"

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
    SortedSignalVector<std::shared_ptr<Account>, SharedPtrElementLess<Account>> accounts_;
};

}  // namespace chatterino
