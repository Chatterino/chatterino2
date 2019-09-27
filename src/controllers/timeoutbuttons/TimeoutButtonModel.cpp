#include "TimeoutButtonModel.hpp"

#include "controllers/timeoutbuttons/TimeoutButton.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

TimeoutButtonModel::TimeoutButtonModel(QObject *parent)
    : SignalVectorModel<TimeoutButton>(2, parent)
{
}

// turn a vector item into a model row
TimeoutButton TimeoutButtonModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const TimeoutButton &original)
{
    return TimeoutButton(row[0]->data(Qt::EditRole).toInt(),
                         row[1]->data(Qt::EditRole).toString());
}

// turns a row in the model into a vector item
void TimeoutButtonModel::getRowFromItem(const TimeoutButton &item,
                                        std::vector<QStandardItem *> &row)
{
    row[0]->setData(item.getDurationString(), Qt::DisplayRole);
    row[1]->setData(item.getUnit(), Qt::DisplayRole);
}

}  // namespace chatterino
