#include "AliasesModel.hpp"

#include "Application.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

AliasesModel::AliasesModel(QObject *parent)
    : SignalVectorModel<AliasesName>(2, parent)
{
}

// turn a vector item into a model row
AliasesName AliasesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                         const AliasesName &original)
{
    return AliasesName{row[0]->data(Qt::DisplayRole).toString(),
                       row[1]->data(Qt::DisplayRole).toString()};
}

// turns a row in the model into a vector item
void AliasesModel::getRowFromItem(const AliasesName &item,
                                  std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getName());
    setStringItem(row[1], item.getReplace());
}

}  // namespace chatterino
