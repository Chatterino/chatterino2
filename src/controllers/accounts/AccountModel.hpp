#pragma once

#include "controllers/accounts/Account.hpp"
#include "util/QstringHash.hpp"
#include "util/SignalVectorModel.hpp"

#include <unordered_map>

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
    virtual std::shared_ptr<Account> getItemFromRow(
        std::vector<QStandardItem *> &row, const std::shared_ptr<Account> &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const std::shared_ptr<Account> &item,
                                std::vector<QStandardItem *> &row) override;

    virtual int beforeInsert(const std::shared_ptr<Account> &item,
                             std::vector<QStandardItem *> &row, int proposedIndex) override;

    virtual void afterRemoved(const std::shared_ptr<Account> &item,
                              std::vector<QStandardItem *> &row, int index) override;

    friend class AccountController;

private:
    std::unordered_map<QString, int> categoryCount;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
