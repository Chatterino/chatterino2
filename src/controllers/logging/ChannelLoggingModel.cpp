#include "controllers/logging/ChannelLoggingModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino {

ChannelLoggingModel ::ChannelLoggingModel(QObject *parent)
    : SignalVectorModel<ChannelLog>(Column::COUNT, parent)
{
}

ChannelLog ChannelLoggingModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const ChannelLog & /*original*/)
{
    return ChannelLog(row[Column::Channel]->data(Qt::DisplayRole).toString());
}

void ChannelLoggingModel::getRowFromItem(const ChannelLog &item,
                                         std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Channel], item.channel);
}

}  // namespace chatterino
