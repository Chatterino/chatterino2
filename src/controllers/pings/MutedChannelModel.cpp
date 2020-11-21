#include "MutedChannelModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

MutedChannelModel::MutedChannelModel(QObject *parent)
    : SignalVectorModel<QString>(1, parent)
{
}

// turn a vector item into a model row
QString MutedChannelModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                          const QString &)
{
    return QString(row[0]->data(Qt::DisplayRole).toString());
}

// turn a model
void MutedChannelModel::getRowFromItem(const QString &item,
                                       std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item);
}

}  // namespace chatterino
