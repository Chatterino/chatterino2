#include "CommandModel.hpp"

namespace chatterino
{
    // commandmodel
    CommandModel::CommandModel(QObject* parent)
        : SignalVectorModel<Command>(2, parent)
    {
    }

    // turn a vector item into a model row
    Command CommandModel::getItemFromRow(
        std::vector<QStandardItem*>& row, const Command& original)
    {
        return Command(row[0]->data(Qt::EditRole).toString(),
            row[1]->data(Qt::EditRole).toString());
    }

    // turns a row in the model into a vector item
    void CommandModel::getRowFromItem(
        const Command& item, std::vector<QStandardItem*>& row)
    {
        row[0]->setData(item.name, Qt::DisplayRole);
        row[0]->setFlags(
            Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        row[1]->setData(item.func, Qt::DisplayRole);
        row[1]->setFlags(
            Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }

}  // namespace chatterino
