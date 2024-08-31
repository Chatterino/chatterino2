#include "controllers/commands/CommandModel.hpp"

#include "common/SignalVector.hpp"
#include "controllers/commands/Command.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
CommandModel::CommandModel(QObject *parent)
    : SignalVectorModel<Command>(Column::COUNT, parent)
{
}

// turn a vector item into a model row
Command CommandModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                     const Command &original)
{
    return Command(row[Column::Trigger]->data(Qt::EditRole).toString(),
                   row[Column::CommandFunc]->data(Qt::EditRole).toString(),
                   row[Column::ShowInMessageContextMenu]
                       ->data(Qt::CheckStateRole)
                       .toBool());
}

// turns a row in the model into a vector item
void CommandModel::getRowFromItem(const Command &item,
                                  std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Trigger], item.name);
    setStringItem(row[Column::CommandFunc], item.func);
    setBoolItem(row[Column::ShowInMessageContextMenu],
                item.showInMsgContextMenu);
}

}  // namespace chatterino
