#include "AccountModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino {

AccountModel::AccountModel(QObject *parent)
    : util::SignalVectorModel<std::shared_ptr<Account>>(1, parent)
{
}

// turn a vector item into a model row
std::shared_ptr<Account> AccountModel::getItemFromRow(std::vector<QStandardItem *> &,
                                                      const std::shared_ptr<Account> &original)
{
    return original;
}

// turns a row in the model into a vector item
void AccountModel::getRowFromItem(const std::shared_ptr<Account> &item,
                                  std::vector<QStandardItem *> &row)
{
    util::setStringItem(row[0], item->toString(), false);
    row[0]->setData(QFont("Segoe UI", 10), Qt::FontRole);
}

int AccountModel::beforeInsert(const std::shared_ptr<Account> &item,
                               std::vector<QStandardItem *> &row, int proposedIndex)
{
    if (this->categoryCount[item->getCategory()]++ == 0) {
        auto row = this->createRow();

        util::setStringItem(row[0], item->getCategory(), false, false);
        row[0]->setData(QFont("Segoe UI Light", 16), Qt::FontRole);

        this->insertCustomRow(std::move(row), proposedIndex);

        return proposedIndex + 1;
    }

    return proposedIndex;
}

void AccountModel::afterRemoved(const std::shared_ptr<Account> &item,
                                std::vector<QStandardItem *> &row, int index)
{
    auto it = this->categoryCount.find(item->getCategory());
    assert(it != this->categoryCount.end());

    if (it->second <= 1) {
        this->categoryCount.erase(it);
        this->removeCustomRow(index - 1);
    } else {
        it->second--;
    }
}

}  // namespace chatterino
