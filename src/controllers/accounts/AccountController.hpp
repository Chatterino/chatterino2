#pragma once

#include "common/Singleton.hpp"

#include <QObject>

#include "common/SignalVector.hpp"
#include "controllers/accounts/Account.hpp"
#include "providers/twitch/TwitchAccountManager.hpp"
#include "util/SharedPtrElementLess.hpp"

namespace chatterino {

class Settings;
class Paths;

class AccountModel;

class AccountController final : public Singleton
{
public:
    AccountController();

    AccountModel *createModel(QObject *parent);

    virtual void initialize(Settings &settings, Paths &paths) override;

    TwitchAccountManager twitch;

private:
    SortedSignalVector<std::shared_ptr<Account>, SharedPtrElementLess<Account>> accounts_;
};

}  // namespace chatterino
