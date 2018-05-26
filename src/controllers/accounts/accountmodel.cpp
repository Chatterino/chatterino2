#include "accountmodel.hpp"

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
}

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
