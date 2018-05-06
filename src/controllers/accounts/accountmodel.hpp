#pragma once

#include "controllers/accounts/account.hpp"
#include "util/signalvectormodel.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

class AccountController;

class AccountModel : public util::SignalVectorModel<std::shared_ptr<Account>>
{
public:
    AccountModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual std::shared_ptr<Account> getItemFromRow(std::vector<QStandardItem *> &row) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const std::shared_ptr<Account> &item,
                                std::vector<QStandardItem *> &row) override;

    friend class AccountController;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
