#include "FilterModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
FilterModel::FilterModel(QObject *parent)
    : SignalVectorModel<FilterRecord>(2, parent)
{
}

// turn a vector item into a model row
FilterRecord FilterModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                         const FilterRecord &original)
{
    return FilterRecord{row[0]->data(Qt::DisplayRole).toString(),
                        row[1]->data(Qt::DisplayRole).toString(),
                        original.getId()};  // persist id
}

// turns a row in the model into a vector item
void FilterModel::getRowFromItem(const FilterRecord &item,
                                 std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getName());
    setStringItem(row[1], item.getFilter());
}

}  // namespace chatterino
