#include "accountmodel.hpp"

#include "util/standarditemhelper.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

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
    row[0]->setData(item->toString(), Qt::DisplayRole);
    row[0]->setData(QFont("Segoe UI", 10), Qt::FontRole);
    //    row[0]->setData(QColor(255, 255, 255), Qt::BackgroundRole);
}

int AccountModel::beforeInsert(const std::shared_ptr<Account> &item,
                               std::vector<QStandardItem *> &row, int proposedIndex)
{
    if (this->categoryCount[item->getCategory()]++ == 0) {
        auto row = this->createRow();

        util::setStringItem(row[0], item->getCategory(), false, false);
        //        row[0]->setData(QColor(142, 36, 170), Qt::ForegroundRole);
        //        row[0]->setData(QColor(255, 255, 255), Qt::BackgroundRole);
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

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
