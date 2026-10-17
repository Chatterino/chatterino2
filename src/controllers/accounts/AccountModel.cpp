// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/accounts/AccountModel.hpp"

#include "controllers/accounts/Account.hpp"
#include "singletons/Theme.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

AccountModel::AccountModel(QObject *parent)
    : SignalVectorModel<std::shared_ptr<Account>>(1, parent)
{
}

// turn a vector item into a model row
std::shared_ptr<Account> AccountModel::getItemFromRow(
    std::vector<QStandardItem *> &, const std::shared_ptr<Account> &original)
{
    return original;
}

// turns a row in the model into a vector item
void AccountModel::getRowFromItem(const std::shared_ptr<Account> &item,
                                  std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item->toString(), false);
    row[0]->setData(QFont("Segoe UI", 10), Qt::FontRole);

    if (item->isExpired())
    {
        row[0]->setData(item->toString() + " (expired)", Qt::DisplayRole);
        row[0]->setData(getTheme()->accounts.expired, Qt::ForegroundRole);
        row[0]->setData(
            "This account's login has expired - sign in again to use it",
            Qt::ToolTipRole);
    }
    else
    {
        row[0]->setData({}, Qt::ForegroundRole);
        row[0]->setData({}, Qt::ToolTipRole);
    }
}

void AccountModel::refreshExpiredState()
{
    int rowIndex = 0;
    for (const auto &row : this->rows())
    {
        if (!row.isCustomRow && row.original)
        {
            // A copy of the pointers is enough - the items they point at are
            // the ones the model reads from
            auto items = row.items;
            this->getRowFromItem(*row.original, items);

            auto index = this->index(rowIndex, 0);
            this->dataChanged(index, index);
        }
        rowIndex++;
    }
}

int AccountModel::beforeInsert(const std::shared_ptr<Account> &item,
                               std::vector<QStandardItem *> &row,
                               int proposedIndex)
{
    if (this->categoryCount_[item->getCategory()]++ == 0)
    {
        auto newRow = this->createRow();

        setStringItem(newRow[0], item->getCategory(), false, false);
        newRow[0]->setData(QFont("Segoe UI Light", 16), Qt::FontRole);

        this->insertCustomRow(std::move(newRow), proposedIndex);

        return proposedIndex + 1;
    }

    return proposedIndex;
}

void AccountModel::afterRemoved(const std::shared_ptr<Account> &item,
                                std::vector<QStandardItem *> &row, int index)
{
    auto it = this->categoryCount_.find(item->getCategory());
    assert(it != this->categoryCount_.end());

    if (it->second <= 1)
    {
        this->categoryCount_.erase(it);
        this->removeCustomRow(index - 1);
    }
    else
    {
        it->second--;
    }
}

}  // namespace chatterino
