#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/accounts/Account.hpp"
#include "util/QStringHash.hpp"

#include <unordered_map>

namespace chatterino {

class AccountController;

class AccountModel : public SignalVectorModel<std::shared_ptr<Account>>
{
public:
    AccountModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual std::shared_ptr<Account> getItemFromRow(
        std::vector<QStandardItem *> &row,
        const std::shared_ptr<Account> &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const std::shared_ptr<Account> &item,
                                std::vector<QStandardItem *> &row) override;

    virtual int beforeInsert(const std::shared_ptr<Account> &item,
                             std::vector<QStandardItem *> &row,
                             int proposedIndex) override;

    virtual void afterRemoved(const std::shared_ptr<Account> &item,
                              std::vector<QStandardItem *> &row,
                              int index) override;

    friend class AccountController;

private:
    std::unordered_map<QString, int> categoryCount_;
};

}  // namespace chatterino
