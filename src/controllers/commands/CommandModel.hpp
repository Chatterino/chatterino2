#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class CommandController;
struct Command;

class CommandModel : public SignalVectorModel<Command>
{
    explicit CommandModel(QObject *parent);

    enum Column {
        Trigger = 0,
        CommandFunc = 1,
        ShowInMessageContextMenu = 2,
        COUNT,
    };

protected:
    // turn a vector item into a model row
    Command getItemFromRow(std::vector<QStandardItem *> &row,
                           const Command &command) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const Command &item,
                        std::vector<QStandardItem *> &row) override;

    friend class CommandController;
};

}  // namespace chatterino
