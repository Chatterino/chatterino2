#include "controllers/nicknames/NicknamesModel.hpp"

#include "controllers/nicknames/Nickname.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

NicknamesModel::NicknamesModel(QObject *parent)
    : SignalVectorModel<Nickname>(4, parent)
{
}

// turn a vector item into a model row
Nickname NicknamesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                        const Nickname &original)
{
    return Nickname{row[0]->data(Qt::DisplayRole).toString().trimmed(),
                    row[1]->data(Qt::DisplayRole).toString(),
                    row[2]->data(Qt::CheckStateRole).toBool(),
                    row[3]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void NicknamesModel::getRowFromItem(const Nickname &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.name());
    setStringItem(row[1], item.replace());
    setBoolItem(row[2], item.isRegex());
    setBoolItem(row[3], item.isCaseSensitive());
}

}  // namespace chatterino
