#include "accountcontroller.hpp"

#include "controllers/accounts/accountmodel.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

AccountController::AccountController()
{
}

void AccountController::load()
{
    this->twitch.load();
}

AccountModel *AccountController::createModel(QObject *parent)
{
    AccountModel *model = new AccountModel(parent);

    //(util::BaseSignalVector<stdAccount> *)
    model->init(&this->accounts);
    return model;
}

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
