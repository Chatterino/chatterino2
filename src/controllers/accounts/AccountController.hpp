#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"

#include <QObject>

namespace chatterino {

class Account;
class Settings;
class Paths;

class AccountModel;

class AccountController final : public Singleton
{
public:
    AccountController();

    AccountModel *createModel(QObject *parent);

    void initialize(Settings &settings, const Paths &paths) override;

    TwitchAccountManager twitch;

private:
    SignalVector<std::shared_ptr<Account>> accounts_;
};

}  // namespace chatterino
