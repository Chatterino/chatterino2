#pragma once

#include <QObject>

#include "controllers/accounts/account.hpp"
#include "providers/twitch/twitchaccountmanager.hpp"
#include "util/sharedptrelementless.hpp"
#include "util/signalvector2.hpp"

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

    providers::twitch::TwitchAccountManager Twitch;

private:
    util::SortedSignalVector<std::shared_ptr<Account>, util::SharedPtrElementLess<Account>>
        accounts;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
