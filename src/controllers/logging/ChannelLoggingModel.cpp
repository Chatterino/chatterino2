#include "controllers/logging/ChannelLoggingModel.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
ChannelLoggingModel ::ChannelLoggingModel(QObject *parent)
    : SignalVectorModel<QString>(1, parent)
{
}

// turn a vector item into a model row
QString ChannelLoggingModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const QString &original)
{
    return QString(row[0]->data(Qt::DisplayRole).toString());
}

// turns a row in the model into a vector item
void ChannelLoggingModel::getRowFromItem(const QString &item,
                                           std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item);
}

}  // namespace chatterino
