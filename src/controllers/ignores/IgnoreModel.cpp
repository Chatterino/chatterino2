#include "IgnoreModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
IgnoreModel::IgnoreModel(QObject *parent)
    : SignalVectorModel<IgnorePhrase>(6, parent)
{
}

// turn a vector item into a model row
IgnorePhrase IgnoreModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                         const IgnorePhrase &original)
{
    // key, regex

    return IgnorePhrase{row[0]->data(Qt::DisplayRole).toString(),
                        row[1]->data(Qt::DisplayRole).toString(),
                        row[2]->data(Qt::CheckStateRole).toBool(),
                        row[4]->data(Qt::CheckStateRole).toBool(),
                        row[5]->data(Qt::DisplayRole).toString(),
                        row[3]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void IgnoreModel::getRowFromItem(const IgnorePhrase &item,
                                 std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setStringItem(row[1], item.getUser());
    setBoolItem(row[2], item.isRegex());
    setBoolItem(row[3], item.isCaseSensitive());
    setBoolItem(row[4], item.isBlock());
    setStringItem(row[5], item.getReplace());
}

}  // namespace chatterino
