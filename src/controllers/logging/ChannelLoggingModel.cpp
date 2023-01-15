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
    auto channelName = row[Column::Channel]->data(Qt::DisplayRole).toString();
    return {channelName};
}

void ChannelLoggingModel::getRowFromItem(const ChannelLog &item,
                                         std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Channel], item.channelName());
}

}  // namespace chatterino
