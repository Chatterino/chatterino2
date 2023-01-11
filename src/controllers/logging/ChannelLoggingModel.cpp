#include "controllers/logging/ChannelLoggingModel.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
ChannelLoggingModel ::ChannelLoggingModel(QObject *parent)
    : SignalVectorModel<ChannelLog>(2, parent)
{
}

// turn a vector item into a model row
ChannelLog ChannelLoggingModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const ChannelLog &channelLog)
{
    return ChannelLog(row[Column::Channel]->data(Qt::DisplayRole).toString(),
                      row[Column::LoggingEnabled]->data(Qt::CheckStateRole).toBool());
}

// turns a row in the model into a vector item
void ChannelLoggingModel::getRowFromItem(const ChannelLog &item,
                                           std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.channel);
    setBoolItem(row[1], item.loggingEnabled);
}

}  // namespace chatterino
