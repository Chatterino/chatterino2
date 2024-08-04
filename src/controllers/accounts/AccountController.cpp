#include "controllers/accounts/AccountController.hpp"

#include "controllers/accounts/Account.hpp"
#include "controllers/accounts/AccountModel.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/SharedPtrElementLess.hpp"

namespace chatterino {

AccountController::AccountController()
    : accounts_(SharedPtrElementLess<Account>{})
{
    // These signal connections can safely be ignored since the twitch object
    // will always be destroyed before the AccountController
    std::ignore =
        this->twitch.accounts.itemInserted.connect([this](const auto &args) {
            this->accounts_.insert(
                std::dynamic_pointer_cast<Account>(args.item));
        });

    std::ignore =
        this->twitch.accounts.itemRemoved.connect([this](const auto &args) {
            if (args.caller != this)
            {
                const auto &accs = this->twitch.accounts.raw();
                auto it = std::find(accs.begin(), accs.end(), args.item);
                assert(it != accs.end());

                this->accounts_.removeAt(it - accs.begin(), this);
            }
        });

    std::ignore = this->accounts_.itemRemoved.connect([this](const auto &args) {
        switch (args.item->getProviderId())
        {
            case ProviderId::Twitch: {
                if (args.caller != this)
                {
                    auto &&accs = this->twitch.accounts;
                    auto it = std::find(accs.begin(), accs.end(), args.item);
                    assert(it != accs.end());
                    this->twitch.accounts.removeAt(it - accs.begin(), this);
                }
            }
            break;
        }
    });
}

void AccountController::load()
{
    this->twitch.load();
}

AccountModel *AccountController::createModel(QObject *parent)
{
    AccountModel *model = new AccountModel(parent);

    model->initialize(&this->accounts_);
    return model;
}

}  // namespace chatterino
