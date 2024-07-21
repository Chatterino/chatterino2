#pragma once

#include "common/SignalVector.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"

#include <QObject>

namespace chatterino {

class Account;
class Settings;
class Paths;

class AccountModel;

class AccountController final
{
public:
    AccountController();

    AccountModel *createModel(QObject *parent);

    /**
     * Load current user & send off a signal to subscribers about any potential changes
     */
    void load();

    TwitchAccountManager twitch;

private:
    SignalVector<std::shared_ptr<Account>> accounts_;
};

}  // namespace chatterino
