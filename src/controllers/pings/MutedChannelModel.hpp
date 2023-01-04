#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class MutedChannelModel : public SignalVectorModel<QString>
{
    explicit MutedChannelModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual QString getItemFromRow(std::vector<QStandardItem *> &row,
                                   const QString &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const QString &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
