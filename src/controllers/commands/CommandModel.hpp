#pragma once

#include <QObject>

#include "controllers/commands/Command.hpp"
#include "common/SignalVectorModel.hpp"

namespace chatterino {

class CommandController;

class CommandModel : public util::SignalVectorModel<Command>
{
    explicit CommandModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual Command getItemFromRow(std::vector<QStandardItem *> &row,
                                   const Command &command) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const Command &item, std::vector<QStandardItem *> &row) override;

    friend class CommandController;
};

}  // namespace chatterino
