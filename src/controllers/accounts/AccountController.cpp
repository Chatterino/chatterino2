// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
            this->accounts_.insert(args.item);
        });

    std::ignore =
        this->twitch.accounts.itemRemoved.connect([this](const auto &args) {
            if (args.caller != this)
            {
                this->accounts_.removeFirstMatching(
                    [&](const auto &item) {
                        return item == args.item;
                    },
                    this);
            }
        });

    std::ignore = this->accounts_.itemRemoved.connect([this](const auto &args) {
        switch (args.item->getProviderId())
        {
            case ProviderId::Twitch: {
                if (args.caller != this)
                {
                    this->twitch.accounts.removeFirstMatching(
                        [&](const auto &item) {
                            return item == args.item;
                        },
                        this);
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
