#include "CommandModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
CommandModel::CommandModel(QObject *parent)
    : SignalVectorModel<Command>(2, parent)
{
}

// turn a vector item into a model row
Command CommandModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                     const Command &original)
{
    return Command(row[0]->data(Qt::EditRole).toString(),
                   row[1]->data(Qt::EditRole).toString());
}

// turns a row in the model into a vector item
void CommandModel::getRowFromItem(const Command &item,
                                  std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.name);
    setStringItem(row[1], item.func);
}

}  // namespace chatterino
