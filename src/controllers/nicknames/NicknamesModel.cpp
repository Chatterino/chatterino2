#include "NicknamesModel.hpp"

#include "Application.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

NicknamesModel::NicknamesModel(QObject *parent)
    : SignalVectorModel<Nickname>(2, parent)
{
}

// turn a vector item into a model row
Nickname NicknamesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                        const Nickname &original)
{
    return Nickname{row[0]->data(Qt::DisplayRole).toString(),
                    row[1]->data(Qt::DisplayRole).toString()};
}

// turns a row in the model into a vector item
void NicknamesModel::getRowFromItem(const Nickname &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.name());
    setStringItem(row[1], item.replace());
}

}  // namespace chatterino
