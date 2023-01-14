#include "controllers/filters/FilterModel.hpp"

#include "Application.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
FilterModel::FilterModel(QObject *parent)
    : SignalVectorModel<FilterRecordPtr>(3, parent)
{
}

// turn a vector item into a model row
FilterRecordPtr FilterModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                            const FilterRecordPtr &original)
{
    auto item =
        std::make_shared<FilterRecord>(row[0]->data(Qt::DisplayRole).toString(),
                                       row[1]->data(Qt::DisplayRole).toString(),
                                       original->getId());  // persist id

    // force 'valid' column to update
    setBoolItem(row[2], item->valid(), false, false);
    setStringItem(row[2], item->valid() ? "Valid" : "Show errors");

    return item;
}

// turns a row in the model into a vector item
void FilterModel::getRowFromItem(const FilterRecordPtr &item,
                                 std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item->getName());
    setStringItem(row[1], item->getFilter());
    setBoolItem(row[2], item->valid(), false, false);
    setStringItem(row[2], item->valid() ? "Valid" : "Show errors");
}

}  // namespace chatterino
