#include "PingModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

PingModel::PingModel(QObject *parent)
    : SignalVectorModel<QString>(1, parent)
{
}

// turn a vector item into a model row
QString PingModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                  const QString &original)
{
    return QString(row[0]->data(Qt::DisplayRole).toString());
}

// turn a model
void PingModel::getRowFromItem(const QString &item,
                               std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item);
}

}  // namespace chatterino
