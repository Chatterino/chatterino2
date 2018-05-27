#include "accountcontroller.hpp"

#include "controllers/accounts/accountmodel.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

AccountController::AccountController()
{
    this->twitch.accounts.itemInserted.connect([this](const auto &args) {
        accounts.insertItem(std::dynamic_pointer_cast<Account>(args.item));
    });
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
