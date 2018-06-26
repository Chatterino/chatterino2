#include "IgnoreModel.hpp"

#include "Application.hpp"
#include "singletons/SettingsManager.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
IgnoreModel::IgnoreModel(QObject *parent)
    : SignalVectorModel<IgnorePhrase>(2, parent)
{
}

// turn a vector item into a model row
IgnorePhrase IgnoreModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                         const IgnorePhrase &original)
{
    // key, regex

    return IgnorePhrase{row[0]->data(Qt::DisplayRole).toString(),
                        row[1]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void IgnoreModel::getRowFromItem(const IgnorePhrase &item, std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.isRegex());
}

}  // namespace chatterino
