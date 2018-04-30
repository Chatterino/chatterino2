#include "commandmodel.hpp"

namespace chatterino {
namespace controllers {
namespace commands {

// commandmodel
CommandModel::CommandModel(QObject *parent)
    : util::SignalVectorModel<Command>(2, parent)
{
}

// turn a vector item into a model row
Command CommandModel::getItemFromRow(std::vector<QStandardItem *> &row)
{
    return Command(row[0]->data(Qt::EditRole).toString(), row[1]->data(Qt::EditRole).toString());
}

// turns a row in the model into a vector item
void CommandModel::getRowFromItem(const Command &item, std::vector<QStandardItem *> &row)
{
    row[0]->setData(item.name, Qt::DisplayRole);
    row[0]->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    row[1]->setData(item.func, Qt::DisplayRole);
    row[1]->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

// returns the related index of the SignalVector
int CommandModel::getVectorIndexFromModelIndex(int index)
{
    return index;
}

// returns the related index of the model
int CommandModel::getModelIndexFromVectorIndex(int index)
{
    return index;
}

}  // namespace commands
}  // namespace controllers
}  // namespace chatterino
