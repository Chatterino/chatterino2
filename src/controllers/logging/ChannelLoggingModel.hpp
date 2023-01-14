#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/logging/ChannelLog.hpp"

#include <QObject>

namespace chatterino {

class ChannelLoggingModel : public SignalVectorModel<ChannelLog>
{
public:
    explicit ChannelLoggingModel(QObject *parent);

    enum Column { Channel = 0, LoggingEnabled = 1 };

protected:
    // turn a vector item into a model row
    ChannelLog getItemFromRow(std::vector<QStandardItem *> &row,
                                      const ChannelLog &channelLog) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const ChannelLog &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
