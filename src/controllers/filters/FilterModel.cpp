#include "FilterModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
FilterModel::FilterModel(QObject *parent)
    : SignalVectorModel<FilterRecord>(3, parent)
{
}

// turn a vector item into a model row
FilterRecord FilterModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                         const FilterRecord &original)
{
    auto item = FilterRecord{row[0]->data(Qt::DisplayRole).toString(),
                             row[1]->data(Qt::DisplayRole).toString(),
                             original.getId()};  // persist id

    // force 'valid' column to update
    setBoolItem(row[2], item.valid(), false, false);
    setStringItem(row[2], item.valid() ? "Valid" : "Show errors");

    return item;
}

// turns a row in the model into a vector item
void FilterModel::getRowFromItem(const FilterRecord &item,
                                 std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getName());
    setStringItem(row[1], item.getFilter());
    setBoolItem(row[2], item.valid(), false, false);
    setStringItem(row[2], item.valid() ? "Valid" : "Show errors");
}

}  // namespace chatterino
